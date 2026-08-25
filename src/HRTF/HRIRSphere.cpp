#include<hgl/audio/HRTF/HRIRSphere.h>
#include<hgl/io/FileInputStream.h>
#include<hgl/utf.h>
#include<algorithm>
#include<cstdio>

namespace hgl::audio
{
    bool HRIRSphereImpl::LoadFromFile(const char *utf8_path)
    {
        if(loaded)
            return true;

        io::OpenFileInputStream file_stream(ToOSString(utf8_path));

        if(!file_stream)
        {
            std::fprintf(stderr,"[HRIRSphere] 无法打开文件: %s\n",utf8_path);
            return false;
        }

        // 读 magic
        file_stream->Read(header.magic,4);

        if(std::strncmp(header.magic,"AMIR",4)!=0)
        {
            std::fprintf(stderr,"[HRIRSphere] 非法 .amir 文件（magic 不符）: %s\n",utf8_path);
            return false;
        }

        // .amir 为小端二进制；FileInputStream 无 ReadUint16/32（那是 DataInputStream），手写 LE 读取
        auto read_le16=[&](uint16 &out)->bool{return file_stream->Read(&out,2)==2;};
        auto read_le32=[&](uint32 &out)->bool{return file_stream->Read(&out,4)==4;};

        if(!read_le16(header.version)||!read_le32(header.sample_rate)
           ||!read_le32(header.ir_length)||!read_le32(header.vertex_count)||!read_le32(header.index_count))
        {
            std::fprintf(stderr,"[HRIRSphere] 头部读取失败\n");
            return false;
        }

        if(header.vertex_count==0||header.index_count==0||header.ir_length==0)
        {
            std::fprintf(stderr,"[HRIRSphere] 数据为空（顶点/索引/IR 长度）\n");
            return false;
        }

        // 索引（三角形）
        std::vector<uint32> indices(header.index_count);
        file_stream->Read(indices.data(),header.index_count*sizeof(uint32));

        // 顶点
        vertices.resize(header.vertex_count);

        for(uint32 i=0;i<header.vertex_count;i++)
        {
            Vertex &v=vertices[i];

            file_stream->Read(&v.position,sizeof(Vec3));

            v.left_ir.resize(header.ir_length);
            file_stream->Read(v.left_ir.data(),header.ir_length*sizeof(float));

            v.right_ir.resize(header.ir_length);
            file_stream->Read(v.right_ir.data(),header.ir_length*sizeof(float));

            file_stream->Read(&v.left_delay,sizeof(float));
            file_stream->Read(&v.right_delay,sizeof(float));
        }

        // 面
        const uint32 face_count=header.index_count/3;
        faces.resize(face_count);

        for(uint32 i=0;i<face_count;i++)
        {
            faces[i].a=indices[i*3+0];
            faces[i].b=indices[i*3+1];
            faces[i].c=indices[i*3+2];
        }

        loaded=true;

        return true;
    }

    bool HRIRSphereImpl::RayTriangleIntersection(
        const Vec3 &origin,const Vec3 &dir,
        const Vec3 &v0,const Vec3 &v1,const Vec3 &v2,Barycentric &out)const
    {
        const Vec3 e1=Vec3Sub(v1,v0);
        const Vec3 e2=Vec3Sub(v2,v0);
        const Vec3 r2=Vec3Cross(dir,e2);

        const float det=Vec3Dot(e1,r2);

        if(det>-kEpsilon&&det<kEpsilon)
            return false; // 射线平行于三角形

        const float inv_det=1.0f/det;
        const Vec3 s=Vec3Sub(origin,v0);
        const float v=inv_det*Vec3Dot(s,r2);

        if(v<-kEpsilon||v>1.0f+kEpsilon)
            return false;

        const Vec3 s1=Vec3Cross(s,e1);
        const float w=inv_det*Vec3Dot(dir,s1);

        if(w<-kEpsilon||v+w>1.0f+kEpsilon)
            return false;

        const float t=inv_det*Vec3Dot(e2,s1);

        if(t>=0.0f)
        {
            out.v=v;
            out.w=w;
            out.u=1.0f-v-w;
            return true;
        }

        return false;
    }

    const HRIRSphereImpl::Face *HRIRSphereImpl::FindFace(const Vec3 &dir)const
    {
        // 线性遍历所有面（顶点数 ≤ 数百，采样发生在配置期非实时）
        for(const Face &face:faces)
        {
            const Vertex &va=vertices[face.a];
            const Vertex &vb=vertices[face.b];
            const Vertex &vc=vertices[face.c];

            Barycentric bary;

            if(RayTriangleIntersection(Vec3{0,0,0},dir,va.position,vb.position,vc.position,bary))
                return &face;
        }

        return nullptr;
    }

    const HRIRSphereImpl::Vertex *HRIRSphereImpl::GetClosestVertex(const Vec3 &position,const Face *face)const
    {
        const Vertex &va=vertices[face->a];
        const Vertex &vb=vertices[face->b];
        const Vertex &vc=vertices[face->c];

        constexpr float k2=kEpsilon*kEpsilon;

        if(Vec3SquaredLength(Vec3Sub(va.position,position))<k2)
            return &va;

        if(Vec3SquaredLength(Vec3Sub(vb.position,position))<k2)
            return &vb;

        if(Vec3SquaredLength(Vec3Sub(vc.position,position))<k2)
            return &vc;

        return nullptr;
    }

    void HRIRSphereImpl::SampleBilinear(const Vec3 &direction,float *left_hrir,float *right_hrir)const
    {
        const Vec3 dir=Vec3Scale(Vec3Normalize(direction),10.0f);
        const Face *face=FindFace(dir);

        if(face==nullptr)
            return;

        // 接近顶点时直接返回该顶点 HRIR
        if(const Vertex *vertex=GetClosestVertex(direction,face))
        {
            std::memcpy(left_hrir,vertex->left_ir.data(),header.ir_length*sizeof(float));
            std::memcpy(right_hrir,vertex->right_ir.data(),header.ir_length*sizeof(float));
            return;
        }

        const Vertex &va=vertices[face->a];
        const Vertex &vb=vertices[face->b];
        const Vertex &vc=vertices[face->c];

        Barycentric bary;

        if(!RayTriangleIntersection(Vec3{0,0,0},dir,va.position,vb.position,vc.position,bary))
            return;

        for(uint32 i=0;i<header.ir_length;i++)
        {
            left_hrir[i]=va.left_ir[i]*bary.u+vb.left_ir[i]*bary.v+vc.left_ir[i]*bary.w;
            right_hrir[i]=va.right_ir[i]*bary.u+vb.right_ir[i]*bary.v+vc.right_ir[i]*bary.w;
        }
    }

    void HRIRSphereImpl::SampleNearestNeighbor(const Vec3 &direction,float *left_hrir,float *right_hrir)const
    {
        const Vec3 dir=Vec3Scale(Vec3Normalize(direction),10.0f);
        const Face *face=FindFace(dir);

        if(face==nullptr)
            return;

        if(const Vertex *vertex=GetClosestVertex(direction,face))
        {
            std::memcpy(left_hrir,vertex->left_ir.data(),header.ir_length*sizeof(float));
            std::memcpy(right_hrir,vertex->right_ir.data(),header.ir_length*sizeof(float));
            return;
        }

        const Vertex &va=vertices[face->a];
        const Vertex &vb=vertices[face->b];
        const Vertex &vc=vertices[face->c];

        Barycentric bary;

        if(!RayTriangleIntersection(Vec3{0,0,0},dir,va.position,vb.position,vc.position,bary))
            return;

        const float min_val=std::min({bary.u,bary.v,bary.w});

        if(min_val==bary.v)
        {
            std::memcpy(left_hrir,va.left_ir.data(),header.ir_length*sizeof(float));
            std::memcpy(right_hrir,va.right_ir.data(),header.ir_length*sizeof(float));
            return;
        }

        if(min_val==bary.u)
        {
            std::memcpy(left_hrir,vb.left_ir.data(),header.ir_length*sizeof(float));
            std::memcpy(right_hrir,vb.right_ir.data(),header.ir_length*sizeof(float));
            return;
        }

        std::memcpy(left_hrir,vc.left_ir.data(),header.ir_length*sizeof(float));
        std::memcpy(right_hrir,vc.right_ir.data(),header.ir_length*sizeof(float));
    }
}//namespace hgl::audio
