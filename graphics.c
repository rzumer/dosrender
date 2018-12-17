#include "graphics.h"

int init_context(GraphicsContext *context)
{
    Coordinates screen_size = { 320, 200 }; /* Screen size in the target 13 hex BIOS mode */
    long buffer_size = ROUND(screen_size.x * screen_size.y);

    context->screen_size = screen_size;
    context->off_screen = (uchar *)(farmalloc(buffer_size));

    if (context->off_screen)
    {
        context->screen = (uchar *)(MK_FP(0xA000, 0));
        _fmemset((void *)(context->off_screen), 0, buffer_size);
        return 1;
    }
    else
    {
        return 0;
    }
}

void free_context(GraphicsContext *context)
{
    /* free owned memory */
    farfree(context->off_screen);

    /* clear the screen content to avoid graphical bugs */
    _fmemset((void *)(context->screen), 0, ROUND(context->screen_size.x * context->screen_size.y));
}

void update_buffer(GraphicsContext *context)
{
    /* wait a full vertical blank before copying */
    while (inportb(INPUT_STATUS) & 8);
    while (!(inportb(INPUT_STATUS) & 8));

    /* copy the off-screen buffer to the video memory */
    _fmemcpy((void *)(context->screen), (void *)(context->off_screen),
        CINT(context->screen_size.x * context->screen_size.y));
}

Polygon clone_polygon(Polygon polygon)
{
    Polygon cloned_polygon = polygon;
    size_t vertices_size = polygon.vertices_length * sizeof(*polygon.vertices);
    cloned_polygon.vertices = malloc(vertices_size);
    memcpy(cloned_polygon.vertices, polygon.vertices, vertices_size);

    return cloned_polygon;
}

Coordinates get_polygon_centroid(Polygon *polygon)
{
    int v;
    double vertices_length = polygon->vertices_length;
    Coordinates centroid;
    long x = 0, y = 0, z = 0;

    for (v = 0; v < vertices_length; v++)
    {
        x += polygon->vertices[v].x;
        y += polygon->vertices[v].y;
        z += polygon->vertices[v].z;
    }

    centroid.x = CROUND(x / vertices_length);
    centroid.y = CROUND(y / vertices_length);
    centroid.z = CROUND(z / vertices_length);

    return centroid;
}

/* Transforms a given vertex based on an origin point and a transformation matrix. */
Coordinates apply_transformation(Coordinates vertex, const Coordinates origin, const Matrix3x3 transformation)
{
    double vertex_data[3] = { 0 };
    Matrix vertex_matrix = { 3, 1, NULL };
    Matrix transformation_matrix = { 3, 3, NULL };

    if (vertex.x != origin.x || vertex.y != origin.y || vertex.z != origin.z)
    {
        vertex_matrix.data = (double *)vertex_data;
        transformation_matrix.data = (double *)transformation.data;

        /* translate such that the origin is at (0, 0) */
        vertex_matrix.data[0] = vertex.x - origin.x;
        vertex_matrix.data[1] = vertex.y - origin.y;
        vertex_matrix.data[2] = vertex.z - origin.z;

        /* matrix operation */
        vertex_matrix = matrix_product(transformation_matrix, vertex_matrix);

        /* translate back to origin-adjusted coordinates */
        vertex.x = CROUND(vertex_matrix.data[0]) + origin.x;
        vertex.y = CROUND(vertex_matrix.data[1]) + origin.y;
        vertex.z = CROUND(vertex_matrix.data[2]) + origin.z;
    }

    return vertex;
}

/* Draws a single point on the screen. */
void draw_point(GraphicsContext *context, Point point)
{
    uchar *buffer; /* points to the screen buffer */
    Coordinates p = point.coordinates;

    if (p.x < 0 || p.y < 0 || p.x >= context->screen_size.x || p.y >= context->screen_size.y)
    {
        return;
    }

    buffer = (uchar *)(context->off_screen + ROUND(p.y * context->screen_size.x + p.x));
    *(buffer) = point.color;
}

/* Draws a straight line between two points, based on Bresenham's algorithm. */
void draw_line(GraphicsContext *context, Line line)
{
    uchar *buffer; /* points to the screen buffer */
    Coordinates delta; /* delta between the points */
    double delta_error; /* error delta per step */
    double error = 0; /* current error */
    long x = CINT(line.a.x); /* horizontal index */
    long y = CINT(line.a.y); /* vertical index */
    int vertical; /* iterate over y instead of x, for lines with large slopes */
    int converged; /* used to determine when to break out of the draw loop */

    delta.x = line.b.x - line.a.x;
    delta.y = line.b.y - line.a.y;

    if ((line.a.x >= context->screen_size.x && line.b.x >= context->screen_size.x) ||
        (line.a.y >= context->screen_size.y && line.b.y >= context->screen_size.y))
    {
        return;
    }

    vertical = cabs(delta.y) > cabs(delta.x);
    delta_error = vertical ? fabs(delta.x / (double)delta.y) : fabs(delta.y / (double)delta.x);

    /* invert the line coordinates if needed */
    if (vertical ? line.b.y < line.a.y : line.b.x < line.a.x)
    {
        Line inverted_line;
        inverted_line.a = line.b;
        inverted_line.b = line.a;
        inverted_line.color = line.color;

        draw_line(context, inverted_line);
        return;
    }

    /* point the buffer to the video memory */
    buffer = (uchar *)(context->off_screen);

    do
    {
        converged = x >= CINT(line.b.x) && y >= CINT(line.b.y);

        if (x >= 0 && y >= 0 && x < context->screen_size.x && y < context->screen_size.y)
        {
            *(buffer + CINT(y * context->screen_size.x + x)) = line.color;
        }

        error += delta_error;

        if (error >= 0.5)
        {
            if (vertical)
            {
                x += SIGN(delta.x);
            }
            else
            {
                y += SIGN(delta.y);
            }

            error -= 1.0;
        }

        vertical ? y++ : x++;
    } while (!converged);
}

/* Draws a rectangle on the screen with arbitrary border and fill colors (0 is transparent). */
void draw_rectangle(GraphicsContext *context, Rectangle rectangle)
{
    Coordinates overflow, underflow; /* used to avoid drawing outside of the screen */
    uchar *buffer; /* points to the screen buffer */
    uchar line_color; /* holds the color (border or fill) used when drawing a horizontal line */
    int y; /* scanline index for the draw loop */
    const int border_size = 1;

    underflow.x = MAX(rectangle.offset.x * -1, 0);
    underflow.y = MAX(rectangle.offset.y * -1, 0);
    overflow.x = MAX((rectangle.offset.x + rectangle.dimensions.x - context->screen_size.x), 0);
    overflow.y = MAX((rectangle.offset.y + rectangle.dimensions.y - context->screen_size.y), 0);

    if (rectangle.offset.x >= context->screen_size.x ||
        rectangle.offset.y >= context->screen_size.y ||
        rectangle.dimensions.x <= 0 ||
        rectangle.dimensions.y <= 0)
    {
        return;
    }

    /* offset the pointer to the video memory */
    buffer = (uchar *)(context->off_screen +
        ROUND(rectangle.offset.y * context->screen_size.x + rectangle.offset.x + underflow.x));

    for (y = -1 * MIN(rectangle.offset.y, 0); y < rectangle.dimensions.y - overflow.y; y++)
    {
        /* draw a full scanline of either the border or the fill color, depending on the current line */
        line_color =
            y == 0 || y == (rectangle.dimensions.y - border_size) ?
            rectangle.border_color : rectangle.fill_color;

        if (line_color)
        {
            /* draw a full horizontal line */
            _fmemset(
                (void *)(buffer + ROUND(y * context->screen_size.x)),
                line_color,
                ROUND(rectangle.dimensions.x - overflow.x - underflow.x - border_size));
        }

        /* draw the border */
        if (rectangle.border_color)
        {
            /* draw vertical borders (two pixels per scanline) only */
            if (rectangle.offset.x >= 0 && rectangle.offset.x < context->screen_size.x)
            {
                *(buffer + ROUND(y * context->screen_size.x)) = rectangle.border_color;
            }

            if (rectangle.offset.x + rectangle.dimensions.x >= 0 &&
                rectangle.offset.x + rectangle.dimensions.x < context->screen_size.x)
            {
                *(buffer +
                    ROUND(y * context->screen_size.x + rectangle.dimensions.x -
                    overflow.x - underflow.x - border_size)) =
                    rectangle.border_color;
            }
        }
    }
}

/* Draws an arbitrary polygon, with a given border color. */
void draw_polygon(GraphicsContext *context, Polygon polygon)
{
    Coordinates *transformed_vertices; /* copy of vertice coordinates post-transformation used for filling */
    coord_t *node_x, swap; /* horizontal node coordinates for a polygon in a scanline, and swap variable for reordering */
    int node_count; /* number of to evaluate during a scanline fill */
    Line line; /* holds parameters used to draw each line of the polygon */
    int v, w, y; /* index iterating over vertices and scanlines */
    Coordinates origin = get_polygon_centroid(&polygon); /* origin point used to apply transformations */
    Coordinates min, max; /* extrema of the polygon image */
    uchar *buffer; /* image buffer where to draw the polygon */

    if (polygon.vertices_length < 3)
    {
        /* not a polygon */
        return;
    }

    if (polygon.fill_color)
    {
        transformed_vertices = malloc(polygon.vertices_length * sizeof(*transformed_vertices));
        node_x = malloc(polygon.vertices_length * sizeof(*node_x));
    }

    for (v = 0; v < polygon.vertices_length; v++)
    {
        line.a = v > 0 ? line.b : polygon.vertices[v];
        line.b = v == polygon.vertices_length - 1 ? polygon.vertices[0] : polygon.vertices[v + 1];
        line.color = polygon.border_color;

        /* apply transformation */
        line.a = v == 0 ? apply_transformation(line.a, origin, polygon.transformation) : line.a;
        line.b = apply_transformation(line.b, origin, polygon.transformation);

        if (polygon.fill_color)
        {
            /* update the polygon's minimum and maximum y coordinates for filling */
            min.y = MIN(line.b.y, min.y);
            max.y = MAX(line.b.y, max.y);

            if (v > 0)
            {
                transformed_vertices[v] = line.a;
            }

            if (v == polygon.vertices_length - 1)
            {
                transformed_vertices[0] = line.b;
            }
        }

        if (polygon.border_color)
        {
            draw_line(context, line);
        }
    }

    if (polygon.fill_color)
    {
        buffer = (uchar *)(context->off_screen);
        min.y = MAX(min.y, 0);
        max.y = MIN(max.y, context->screen_size.y - 1);

        for (y = CINT(min.y); y < CINT(max.y); y++)
        {
            node_count = 0;
            w = polygon.vertices_length - 1;

            for (v = 0; v < polygon.vertices_length; v++)
            {
                if (transformed_vertices[v].y < y && transformed_vertices[w].y >= y ||
                    transformed_vertices[w].y < y && transformed_vertices[v].y >= y)
                {
                    node_x[node_count++] = ROUND(transformed_vertices[v].x +
                        (y - transformed_vertices[v].y) /
                        (double)(transformed_vertices[w].y - transformed_vertices[v].y) *
                        (transformed_vertices[w].x - transformed_vertices[v].x));
                }

                w = v;
            }

            v = 0;
            while (v < node_count - 1)
            {
                if (node_x[v] > node_x[v + 1])
                {
                    swap = node_x[v];
                    node_x[v] = node_x[v + 1];
                    node_x[v + 1] = swap;

                    if (v)
                        v--;
                }
                else
                {
                    v++;
                }
            }

            for (v = 0; v < node_count; v += 2)
            {
                if (node_x[v] >= context->screen_size.x)
                    break;

                if (node_x[v + 1] > 0)
                {
                    min.x = MAX(node_x[v], 0);
                    max.x = MIN(node_x[v + 1], context->screen_size.x - 1);

                    _fmemset(buffer + CINT(y * context->screen_size.x + min.x), polygon.fill_color, CINT(max.x - min.x));
                }
            }
        }

        free(transformed_vertices);
        free(node_x);
    }
}

/* Scales a vertex around an origin point. */
Coordinates scale_vertex(Coordinates vertex, Coordinates origin, double scale_x, double scale_y)
{
    Coordinates relative_vertex, scaled_vertex;

    relative_vertex.x = vertex.x - origin.x;
    relative_vertex.y = vertex.y - origin.y;

    scaled_vertex.x = CROUND(relative_vertex.x * scale_x) + origin.x;
    scaled_vertex.y = CROUND(relative_vertex.y * scale_y) + origin.y;

    return scaled_vertex;
}

/* Scales a line around its origin. Negative scale factors allow mirroring. */
Line scale_line(Line line, double scale_x, double scale_y)
{
    Line scaled_line = line;
    scaled_line.b = scale_vertex(line.b, line.a, scale_x, scale_y);

    return scaled_line;
}

/* Scales a rectangle around its origin. Negative scale factors allow mirroring. */
Rectangle scale_rectangle(Rectangle rectangle, double scale_x, double scale_y)
{
    Rectangle scaled_rectangle;
    int offset; /* holds the offset when swapping values due to mirroring */

    scaled_rectangle.offset = rectangle.offset;
    scaled_rectangle.dimensions.x = rectangle.dimensions.x * scale_x;
    scaled_rectangle.dimensions.y = rectangle.dimensions.y * scale_y;
    scaled_rectangle.border_color = rectangle.border_color;
    scaled_rectangle.fill_color = rectangle.fill_color;

    /* handle mirroring */
    if (scale_x < 0)
    {
        offset = scaled_rectangle.offset.x;
        scaled_rectangle.offset.x += scaled_rectangle.dimensions.x;
        scaled_rectangle.dimensions.x = offset - scaled_rectangle.offset.x;
    }

    if (scale_y < 0)
    {
        offset = scaled_rectangle.offset.y;
        scaled_rectangle.offset.y += scaled_rectangle.dimensions.y;
        scaled_rectangle.dimensions.y = offset - scaled_rectangle.offset.y;
    }

    return scaled_rectangle;
}

Polygon scale_polygon(Polygon polygon, double scale_x, double scale_y)
{
    Polygon scaled_polygon = polygon;

    Matrix3x3 scaling_transformation = { 0 };
    scaling_transformation.data[0][0] = scale_x;
    scaling_transformation.data[1][1] = scale_y;
    scaling_transformation.data[2][2] = 1.0;

    scaled_polygon.transformation = matrix3x3_product(scaling_transformation, polygon.transformation);

    return scaled_polygon;
}

/* Rotates a vertex in the 2D plane around an origin point. */
Coordinates rotate_vertex(Coordinates vertex, Coordinates origin, double angle)
{
    Coordinates relative_vertex, rotated_vertex;
    double radians = angle * M_PI / 180.0;

    relative_vertex.x = vertex.x - origin.x;
    relative_vertex.y = vertex.y - origin.y;

    rotated_vertex.x = CROUND(relative_vertex.x * cos(radians) - relative_vertex.y * sin(radians)) + origin.x;
    rotated_vertex.y = CROUND(relative_vertex.y * cos(radians) + relative_vertex.x * sin(radians)) + origin.y;

    return rotated_vertex;
}

/* Rotates a line around its origin. */
Line rotate_line(Line line, double angle)
{
    Line rotated_line = line;
    rotated_line.b = rotate_vertex(line.b, line.a, angle);

    return rotated_line;
}

Polygon rotate_polygon(Polygon polygon, double angle, Axis axis)
{
    Polygon rotated_polygon = polygon;
    double radians = angle * M_PI / 180.0;

    Matrix3x3 rotation_transformation = MATRIX_3X3_IDENTITY;

    switch (axis)
    {
        case AXIS_X:
        rotation_transformation.data[1][1] = cos(radians);
        rotation_transformation.data[1][2] = -sin(radians);
        rotation_transformation.data[2][1] = sin(radians);
        rotation_transformation.data[2][2] = cos(radians);
        break;
        case AXIS_Y:
        rotation_transformation.data[2][2] = cos(radians);
        rotation_transformation.data[2][0] = -sin(radians);
        rotation_transformation.data[0][2] = sin(radians);
        rotation_transformation.data[0][0] = cos(radians);
        break;
        case AXIS_Z:
        rotation_transformation.data[0][0] = cos(radians);
        rotation_transformation.data[0][1] = -sin(radians);
        rotation_transformation.data[1][0] = sin(radians);
        rotation_transformation.data[1][1] = cos(radians);
        break;
    }

    rotated_polygon.transformation = matrix3x3_product(rotation_transformation, polygon.transformation);

    return rotated_polygon;
}

/* Shears a vertex around an origin point. */
Coordinates shear_vertex(Coordinates vertex, Coordinates origin, double shear_x, double shear_y)
{
    Coordinates relative_vertex, shorn_vertex;

    relative_vertex.x = vertex.x - origin.x;
    relative_vertex.y = vertex.y - origin.y;

    shorn_vertex.x = CROUND(relative_vertex.x + shear_x * relative_vertex.y) + origin.x;
    shorn_vertex.y = CROUND(relative_vertex.y + shear_y * relative_vertex.x) + origin.y;

    return shorn_vertex;
}

/* Shears a line around its origin. */
Line shear_line(Line line, double shear_x, double shear_y)
{
    Line shorn_line = line;
    shorn_line.b = shear_vertex(line.b, line.a, shear_x, shear_y);

    return shorn_line;
}

Polygon shear_polygon(Polygon polygon, double shear_x, double shear_y)
{
    Polygon shorn_polygon = polygon;

    Matrix3x3 shear_transformation = MATRIX_3X3_IDENTITY;
    shear_transformation.data[0][1] = shear_x;
    shear_transformation.data[1][0] = shear_y;

    shorn_polygon.transformation = matrix3x3_product(shear_transformation, polygon.transformation);

    return shorn_polygon;
}
