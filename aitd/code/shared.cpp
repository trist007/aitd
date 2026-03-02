// Defines
#define MAX_BONES        64
#define MAX_KEYFRAMES    256

#ifdef DEBUG
#define DebugLog(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DebugLog(fmt, ...) do { char _buf[256]; sprintf_s(_buf, sizeof(_buf), fmt, __VA_ARGS__); OutputDebugStringA(_buf); } while(0)
#endif

struct Mat4
{
    float        m[16]; // columm major
};

struct BoneWeight
{
    int          joints[4];
    float        weights[4];
};

struct Bone
{
    char         name[64];
    int          parent;     // -1 if root
    Mat4         inv_bind;   // inverse bind pose matrix
};

struct Keyframe
{
    float        time;
    float        value[4];  // vec3 for translation/scale, quaternion for rotation
};

struct AnimChannel
{
    int          bone_index;
    int          type;    // 0=translation, 1=rotation, 2=scale
    Keyframe    *keyframes;
    int          keyframe_count;
};

struct Animation
{
    char         name[64];
    AnimChannel *channels;
    int          channel_count;
    float        duration;
};

struct MDLVertex
{
    float        x, y, z;
};

struct MDLFace
{
    unsigned int v0, v1, v2;
    float        u[3], v[3];
    float        r, g, b;
};

struct Model
{
    // Mesh
    MDLVertex   *vertices;
    MDLFace     *faces;
    int          vertex_count;
    int          face_count;
    GLuint       texture;
    
    // Vertex Skinning (armature)
    BoneWeight  *bone_weights; // one per vertex
    Bone         bones[MAX_BONES];
    int          bone_count;
    Mat4         bone_matrices[MAX_BONES];  // computed each frame
    
    // Animations
    Animation   *animations;
    int          animation_count;
};

static void
Mat4FromCgltf(Mat4 *out, const float *m)
{
    for(int i = 0;
        i < 16;
        i++)
        out->m[i] = m[i];
}

static int
FindBoneIndex(Model *mode, cgltf_node **joints, int joint_count, cgltf_node *node)
{
    for(int i = 0;
        i < joint_count;
        i++)
    {
        if(joints[i] == node)
            return(i);
    }
    return(-1);
}

Model
BuildModelFromGLB(const char *filename)
{
    Model model = {};
    
    cgltf_options options = {};
    cgltf_data   *data    = NULL;
    
    if(cgltf_parse_file(&options, filename, &data) != cgltf_result_success)
    {
        OutputDebugStringA("Failed to parse GLB\n");
        return model;
    }
    
    if(cgltf_load_buffers(&options, data, filename) != cgltf_result_success)
    {
        OutputDebugStringA("Failed to load GLB buffers\n");
        cgltf_free(data);
        return model;
    }
    
    DebugLog("meshes: %d primitives: %d\n", (int)data->meshes_count, (int)data->meshes[0].primitives_count);
    // ----------------------------------------------------------------
    // TEXTURE
    // ----------------------------------------------------------------
    if(data->images_count > 0)
    {
        cgltf_image *image = &data->images[1];
        cgltf_buffer_view *bv = image->buffer_view;
        DebugLog("image_count: %d\n", (int)data->images_count);
        
        unsigned char *raw = (unsigned char *)bv->buffer->data + bv->offset;
        int width, height, channels;
        unsigned char *pixels = stbi_load_from_memory(raw, (int)bv->size,
                                                      &width, &height, &channels, 3);
        if(pixels)
        {
            glGenTextures(1, &model.texture);
            glBindTexture(GL_TEXTURE_2D, model.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
            stbi_image_free(pixels);
        }
        else
        {
            OutputDebugStringA("Failed to decode texture from GLB\n");
        }
    }
    
    // ----------------------------------------------------------------
    // MESH
    // ----------------------------------------------------------------
    if(data->meshes_count == 0)
    {
        OutputDebugStringA("No meshes in GLB\n");
        cgltf_free(data);
        return model;
    }
    
    cgltf_primitive *prim = &data->meshes[0].primitives[0];
    
    // count vertices
    for(int i = 0; i < (int)prim->attributes_count; i++)
    {
        if(prim->attributes[i].type == cgltf_attribute_type_position)
        {
            char debug[128];
            model.vertex_count = (int)prim->attributes[i].data->count;
            sprintf_s(debug, sizeof(debug), "vertex_count: %d attributes_count: %d\n",
                      model.vertex_count, (int)prim->attributes_count);
            OutputDebugStringA(debug);
            break;
        }
    }
    
    // count faces
    if(prim->indices)
        model.face_count = (int)prim->indices->count / 3;
    else
        model.face_count = model.vertex_count / 3;
    
    DebugLog("face_count: %d\n", model.face_count);
    
    model.vertices    = (MDLVertex  *)malloc(sizeof(MDLVertex)  * model.vertex_count);
    model.faces       = (MDLFace    *)malloc(sizeof(MDLFace)    * model.face_count);
    model.bone_weights= (BoneWeight *)malloc(sizeof(BoneWeight) * model.vertex_count);
    memset(model.bone_weights, 0, sizeof(BoneWeight) * model.vertex_count);
    
    // read attributes
    for(int i = 0; i < (int)prim->attributes_count; i++)
    {
        cgltf_attribute *attr = &prim->attributes[i];
        cgltf_accessor  *acc  = attr->data;
        
        if(attr->type == cgltf_attribute_type_position)
        {
            for(int v = 0; v < model.vertex_count; v++)
            {
                float pos[3];
                cgltf_accessor_read_float(acc, v, pos, 3);
                model.vertices[v].x = pos[0];
                model.vertices[v].y = pos[1];
                model.vertices[v].z = pos[2];
            }
        }
        else if(attr->type == cgltf_attribute_type_texcoord)
        {
            // store UVs temporarily — we'll assign per-face below
            // for now store in a temp array
            float *uvs = (float *)malloc(sizeof(float) * 2 * model.vertex_count);
            for(int v = 0; v < model.vertex_count; v++)
            {
                float uv[2];
                cgltf_accessor_read_float(acc, v, uv, 2);
                uvs[v*2 + 0] = uv[0];
                //uvs[v*2 + 1] = 1.0f - uv[1]; // flip Y like you did in OBJ loader
                uvs[v*2 + 1] = uv[1]; // flip Y like you did in OBJ loader
            }
            
            // assign UVs to faces
            if(prim->indices)
            {
                for(int f = 0; f < model.face_count; f++)
                {
                    for(int c = 0; c < 3; c++)
                    {
                        int idx = (int)cgltf_accessor_read_index(prim->indices, f*3 + c);
                        model.faces[f].u[c] = uvs[idx*2 + 0];
                        model.faces[f].v[c] = uvs[idx*2 + 1];
                    }
                }
            }
            else
            {
                for(int f = 0; f < model.face_count; f++)
                {
                    for(int c = 0; c < 3; c++)
                    {
                        int idx = f*3 + c;
                        model.faces[f].u[c] = uvs[idx*2 + 0];
                        model.faces[f].v[c] = uvs[idx*2 + 1];
                    }
                }
            }
            free(uvs);
        }
        else if(attr->type == cgltf_attribute_type_joints)
        {
            for(int v = 0; v < model.vertex_count; v++)
            {
                cgltf_uint j[4];
                cgltf_accessor_read_uint(acc, v, j, 4);
                model.bone_weights[v].joints[0] = (int)j[0];
                model.bone_weights[v].joints[1] = (int)j[1];
                model.bone_weights[v].joints[2] = (int)j[2];
                model.bone_weights[v].joints[3] = (int)j[3];
            }
        }
        else if(attr->type == cgltf_attribute_type_weights)
        {
            for(int v = 0; v < model.vertex_count; v++)
            {
                float w[4];
                cgltf_accessor_read_float(acc, v, w, 4);
                model.bone_weights[v].weights[0] = w[0];
                model.bone_weights[v].weights[1] = w[1];
                model.bone_weights[v].weights[2] = w[2];
                model.bone_weights[v].weights[3] = w[3];
            }
        }
    }
    for(int i = 0; i < 10; i++)
    {
        DebugLog("v[%d]: %f %f %f\n", i, model.vertices[i].x, model.vertices[i].y, model.vertices[i].z);
    }
    
    // read indices -> faces
    if(prim->indices)
    {
        for(int f = 0; f < model.face_count; f++)
        {
            model.faces[f].v0 = (unsigned int)cgltf_accessor_read_index(prim->indices, f*3 + 0);
            model.faces[f].v1 = (unsigned int)cgltf_accessor_read_index(prim->indices, f*3 + 1);
            model.faces[f].v2 = (unsigned int)cgltf_accessor_read_index(prim->indices, f*3 + 2);
            model.faces[f].r  = 1.0f;
            model.faces[f].g  = 1.0f;
            model.faces[f].b  = 1.0f;
        }
    }
    else
    {
        // no index buffer, vertices are already in order
        for(int f = 0; f < model.face_count; f++)
        {
            model.faces[f].v0 = f*3 + 0;
            model.faces[f].v1 = f*3 + 1;
            model.faces[f].v2 = f*3 + 2;
            model.faces[f].r  = 1.0f;
            model.faces[f].g  = 1.0f;
            model.faces[f].b  = 1.0f;
        }
    }
    
    // ----------------------------------------------------------------
    // SKINNING
    // ----------------------------------------------------------------
    if(data->skins_count > 0)
    {
        cgltf_skin *skin     = &data->skins[0];
        model.bone_count     = (int)skin->joints_count;
        
        for(int i = 0; i < model.bone_count; i++)
        {
            cgltf_node *joint = skin->joints[i];
            
            strncpy(model.bones[i].name, joint->name ? joint->name : "", 63);
            
            // find parent index
            model.bones[i].parent = -1;
            if(joint->parent)
                model.bones[i].parent = FindBoneIndex(&model, skin->joints, model.bone_count, joint->parent);
            
            // inverse bind matrix
            if(skin->inverse_bind_matrices)
            {
                float m[16];
                cgltf_accessor_read_float(skin->inverse_bind_matrices, i, m, 16);
                Mat4FromCgltf(&model.bones[i].inv_bind, m);
            }
        }
    }
    
    for(int i = 0; i < 4; i++)
    {
        cgltf_node *joint = data->skins[0].joints[i];
        DebugLog("joint[%d] %s local t: %f %f %f\n", i,
                 joint->name ? joint->name : "null",
                 joint->translation[0],
                 joint->translation[1],
                 joint->translation[2]);
    }
    
    
    // ----------------------------------------------------------------
    // ANIMATIONS
    // ----------------------------------------------------------------
    if(data->animations_count > 0)
    {
        model.animation_count = (int)data->animations_count;
        model.animations      = (Animation *)malloc(sizeof(Animation) * model.animation_count);
        memset(model.animations, 0, sizeof(Animation) * model.animation_count);
        
        for(int a = 0; a < model.animation_count; a++)
        {
            cgltf_animation *src_anim = &data->animations[a];
            Animation       *dst_anim = &model.animations[a];
            
            strncpy(dst_anim->name, src_anim->name ? src_anim->name : "anim", 63);
            
            dst_anim->channel_count = (int)src_anim->channels_count;
            dst_anim->channels      = (AnimChannel *)malloc(sizeof(AnimChannel) * dst_anim->channel_count);
            memset(dst_anim->channels, 0, sizeof(AnimChannel) * dst_anim->channel_count);
            
            for(int c = 0; c < dst_anim->channel_count; c++)
            {
                cgltf_animation_channel *src_ch = &src_anim->channels[c];
                AnimChannel             *dst_ch = &dst_anim->channels[c];
                
                // which bone does this channel drive
                if(data->skins_count > 0)
                    dst_ch->bone_index = FindBoneIndex(&model,
                                                       data->skins[0].joints,
                                                       model.bone_count,
                                                       src_ch->target_node);
                
                // translation / rotation / scale
                switch(src_ch->target_path)
                {
                    case cgltf_animation_path_type_translation: dst_ch->type = 0; break;
                    case cgltf_animation_path_type_rotation:    dst_ch->type = 1; break;
                    case cgltf_animation_path_type_scale:       dst_ch->type = 2; break;
                    default:                                     dst_ch->type = -1; break;
                }
                
                cgltf_accessor *times  = src_ch->sampler->input;
                cgltf_accessor *values = src_ch->sampler->output;
                
                dst_ch->keyframe_count = (int)times->count;
                dst_ch->keyframes      = (Keyframe *)malloc(sizeof(Keyframe) * dst_ch->keyframe_count);
                
                for(int k = 0; k < dst_ch->keyframe_count; k++)
                {
                    cgltf_accessor_read_float(times,  k, &dst_ch->keyframes[k].time, 1);
                    int components = (dst_ch->type == 1) ? 4 : 3; // rotation=quat, others=vec3
                    cgltf_accessor_read_float(values, k, dst_ch->keyframes[k].value, components);
                    
                    // track animation duration
                    if(dst_ch->keyframes[k].time > dst_anim->duration)
                        dst_anim->duration = dst_ch->keyframes[k].time;
                }
            }
        }
    }
    
    for(int i = 0; i < model.animation_count; i++)
        DebugLog("anim[%d] name=%s duration=%f\n", i, model.animations[i].name, model.animations[i].duration);
    
    Animation *anim = &model.animations[0];
    
    for(int c = 0; c < anim->channel_count; c++)
    {
        AnimChannel *ch = &anim->channels[c];
        DebugLog("channel[%d] bone=%d type=%d keyframes=%d\n",
                 c, ch->bone_index, ch->type, ch->keyframe_count);
    }
    
    cgltf_free(data);
    return model;
}

static float
LerpFloat(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void
SlerpQuat(float *out, float *a, float *b, float t)
{
    float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
    
    // if dot is negative flip b to take shortest path
    if(dot < 0.0f)
    {
        b[0] = -b[0]; b[1] = -b[1]; b[2] = -b[2]; b[3] = -b[3];
        dot = -dot;
    }
    
    if(dot > 0.9995f)
    {
        // close enough, just lerp
        out[0] = LerpFloat(a[0], b[0], t);
        out[1] = LerpFloat(a[1], b[1], t);
        out[2] = LerpFloat(a[2], b[2], t);
        out[3] = LerpFloat(a[3], b[3], t);
        return;
    }
    
    float angle  = acosf(dot);
    float s      = sinf(angle);
    float ta     = sinf((1.0f - t) * angle) / s;
    float tb     = sinf(t * angle) / s;
    
    out[0] = ta*a[0] + tb*b[0];
    out[1] = ta*a[1] + tb*b[1];
    out[2] = ta*a[2] + tb*b[2];
    out[3] = ta*a[3] + tb*b[3];
}

static void
Mat4Identity(Mat4 *m)
{
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

static void
Mat4Multiply(Mat4 *out, Mat4 *a, Mat4 *b)
{
    Mat4 result = {};
    for(int col = 0; col < 4; col++)
    {
        for(int row = 0; row < 4; row++)
        {
            float sum = 0.0f;
            for(int k = 0; k < 4; k++)
                sum += a->m[k*4 + row] * b->m[col*4 + k];
            result.m[col*4 + row] = sum;
        }
    }
    *out = result;
}

static void
Mat4FromTRS(Mat4 *out, float *t, float *r, float *s)
{
    float x = r[0], y = r[1], z = r[2], w = r[3];
    
    // rotation matrix from quaternion (row major)
    float r00 = 1 - 2*(y*y + z*z);
    float r01 = 2*(x*y - w*z);
    float r02 = 2*(x*z + w*y);
    
    float r10 = 2*(x*y + w*z);
    float r11 = 1 - 2*(x*x + z*z);
    float r12 = 2*(y*z - w*x);
    
    float r20 = 2*(x*z - w*y);
    float r21 = 2*(y*z + w*x);
    float r22 = 1 - 2*(x*x + y*y);
    
    // column major storage
    out->m[0]  = s[0] * r00;
    out->m[1]  = s[0] * r10;
    out->m[2]  = s[0] * r20;
    out->m[3]  = 0.0f;
    
    out->m[4]  = s[1] * r01;
    out->m[5]  = s[1] * r11;
    out->m[6]  = s[1] * r21;
    out->m[7]  = 0.0f;
    
    out->m[8]  = s[2] * r02;
    out->m[9]  = s[2] * r12;
    out->m[10] = s[2] * r22;
    out->m[11] = 0.0f;
    
    out->m[12] = t[0];
    out->m[13] = t[1];
    out->m[14] = t[2];
    out->m[15] = 1.0f;
}

static void
SampleChannel(AnimChannel *ch, float time, float *out)
{
    if(ch->keyframe_count == 0) return;
    
    // single keyframe - just use it
    if(ch->keyframe_count == 1)
    {
        memcpy(out, ch->keyframes[0].value, sizeof(float) * 4);
        return;
    }
    
    // clamp to last keyframe
    if(time >= ch->keyframes[ch->keyframe_count - 1].time)
    {
        memcpy(out, ch->keyframes[ch->keyframe_count - 1].value, sizeof(float) * 4);
        return;
    }
    
    // find surrounding keyframes
    for(int k = 0; k < ch->keyframe_count - 1; k++)
    {
        float t0 = ch->keyframes[k].time;
        float t1 = ch->keyframes[k + 1].time;
        
        if(time >= t0 && time < t1)
        {
            float t = (time - t0) / (t1 - t0);
            
            if(ch->type == 1) // rotation - slerp
                SlerpQuat(out, ch->keyframes[k].value, ch->keyframes[k+1].value, t);
            else // translation/scale - lerp
            {
                out[0] = LerpFloat(ch->keyframes[k].value[0], ch->keyframes[k+1].value[0], t);
                out[1] = LerpFloat(ch->keyframes[k].value[1], ch->keyframes[k+1].value[1], t);
                out[2] = LerpFloat(ch->keyframes[k].value[2], ch->keyframes[k+1].value[2], t);
            }
            return;
        }
    }
}

void
UpdateAnimation(Model *model, int anim_index, float time)
{
    // initialize all bone matrices to identity first
    for(int i = 0; i < model->bone_count; i++)
        Mat4Identity(&model->bone_matrices[i]);
    
    if(anim_index >= model->animation_count) return;
    
    Animation *anim = &model->animations[anim_index];
    
    // loop the animation
    time = fmodf(time, anim->duration);
    
    // local bone transforms
    float translations[MAX_BONES][3] = {};
    float rotations[MAX_BONES][4]    = {};
    float scales[MAX_BONES][3]       = {};
    
    // default scale to 1
    for(int i = 0; i < model->bone_count; i++)
    {
        scales[i][0] = scales[i][1] = scales[i][2] = 1.0f;
        rotations[i][3] = 1.0f; // identity quaternion
    }
    
    // sample all channels
    for(int c = 0; c < anim->channel_count; c++)
    {
        AnimChannel *ch = &anim->channels[c];
        int b = ch->bone_index;
        if(b < 0 || b >= model->bone_count) continue;
        
        if(ch->type == 0)      SampleChannel(ch, time, translations[b]);
        else if(ch->type == 1) SampleChannel(ch, time, rotations[b]);
        else if(ch->type == 2) SampleChannel(ch, time, scales[b]);
    }
    
    // compute local bone matrices
    Mat4 local[MAX_BONES];
    for(int i = 0; i < model->bone_count; i++)
        Mat4FromTRS(&local[i], translations[i], rotations[i], scales[i]);
    
    // compute global bone matrices (parent * local)
    Mat4 global[MAX_BONES];
    for(int i = 0; i < model->bone_count; i++)
    {
        int parent = model->bones[i].parent;
        if(parent < 0)
            global[i] = local[i];
        else
            Mat4Multiply(&global[i], &global[parent], &local[i]);
    }
    // final bone matrix = global * inverse_bind
    for(int i = 0; i < model->bone_count; i++)
        Mat4Multiply(&model->bone_matrices[i], &global[i], &model->bones[i].inv_bind);
}