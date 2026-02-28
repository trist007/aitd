#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct
{
    char     magic[4];
    unsigned int version;
    unsigned int vertex_count;
    unsigned int face_count;
} MDLHeader;

typedef struct
{
    float x, y, z;
} MDLVertex;

typedef struct
{
    unsigned int v0, v1, v2;
    float r, g, b;
} MDLFace;
#pragma pack(pop)

#define MAX_VERTICES 1024
#define MAX_FACES    1024

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: obj_to_mdl input.obj output.mdl\n");
        return 1;
    }
    
    FILE *obj = fopen(argv[1], "r");
    if(!obj)
    {
        printf("Failed to open %s\n", argv[1]);
        return 1;
    }
    
    MDLVertex vertices[MAX_VERTICES];
    MDLFace   faces[MAX_FACES];
    int vertex_count = 0;
    int face_count   = 0;
    
    char line[256];
    while(fgets(line, sizeof(line), obj))
    {
        if(line[0] == 'v' && line[1] == ' ')
        {
            float x, y, z;
            sscanf_s(line, "v %f %f %f", &x, &y, &z);
            vertices[vertex_count].x = x;
            vertices[vertex_count].y = y;
            vertices[vertex_count].z = z;
            vertex_count++;
        }
        else if(line[0] == 'f' && line[1] == ' ')
        {
            int v0, v1, v2;
            if(sscanf_s(line, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &v0, &v1, &v2) == 3 ||
               sscanf_s(line, "f %d//%*d %d//%*d %d//%*d", &v0, &v1, &v2) == 3 ||
               sscanf_s(line, "f %d/%*d %d/%*d %d/%*d", &v0, &v1, &v2) == 3 ||
               sscanf_s(line, "f %d %d %d", &v0, &v1, &v2) == 3)
            {
                faces[face_count].v0 = v0 - 1;  /* OBJ is 1 indexed */
                faces[face_count].v1 = v1 - 1;
                faces[face_count].v2 = v2 - 1;
                faces[face_count].r  = 0.8f;    /* default grey suit color */
                faces[face_count].g  = 0.7f;
                faces[face_count].b  = 0.6f;
                face_count++;
            }
        }
    }
    fclose(obj);
    
    printf("Vertices: %d Faces: %d\n", vertex_count, face_count);
    
    FILE *mdl = fopen(argv[2], "wb");
    if(!mdl)
    {
        printf("Failed to open %s\n", argv[2]);
        return 1;
    }
    
    MDLHeader header;
    header.magic[0]    = 'M';
    header.magic[1]    = 'D';
    header.magic[2]    = 'L';
    header.magic[3]    = 'A';
    header.version     = 1;
    header.vertex_count = vertex_count;
    header.face_count   = face_count;
    
    fwrite(&header,   sizeof(MDLHeader), 1,            mdl);
    fwrite(vertices,  sizeof(MDLVertex), vertex_count, mdl);
    fwrite(faces,     sizeof(MDLFace),   face_count,   mdl);
    
    fclose(mdl);
    
    printf("Written to %s\n", argv[2]);
    return 0;
}