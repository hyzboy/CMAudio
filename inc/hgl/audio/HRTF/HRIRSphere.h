#pragma once

#include<hgl/audio/Ambisonics/AmbisonicTypes.h>
#include<vector>
#include<cstring>

namespace hgl::audio
{
    // ===== Vec3 数学扩展（HRIR 采样用）=====
    inline Vec3 Vec3Add(const Vec3 &a,const Vec3 &b){return Vec3{a.x+b.x,a.y+b.y,a.z+b.z};}
    inline Vec3 Vec3Sub(const Vec3 &a,const Vec3 &b){return Vec3{a.x-b.x,a.y-b.y,a.z-b.z};}
    inline Vec3 Vec3Scale(const Vec3 &a,float s){return Vec3{a.x*s,a.y*s,a.z*s};}
    inline float Vec3Dot(const Vec3 &a,const Vec3 &b){return a.x*b.x+a.y*b.y+a.z*b.z;}
    inline Vec3 Vec3Cross(const Vec3 &a,const Vec3 &b)
    {
        return Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
    }
    inline float Vec3SquaredLength(const Vec3 &a){return Vec3Dot(a,a);}
    inline Vec3 Vec3Normalize(const Vec3 &a)
    {
        const float len=std::sqrt(Vec3Dot(a,a));

        if(len<1e-12f)
            return Vec3{0.0f,0.0f,0.0f};

        return Vec3{a.x/len,a.y/len,a.z/len};
    }

    /**
    * HRIR 球面数据（R3 实现，源自 Amplitude Audio SDK HRIRSphere，Apache 2.0）
    *
    * 加载 .amir 二进制数据（球面顶点 + 三角面 + 每顶点左右耳 HRIR），
    * 按方向采样（最近邻 / 双线性重心插值）。
    */
    class HRIRSphere
    {
    public:
        enum class SamplingMode
        {
            NearestNeighbor,
            Bilinear
        };

        virtual ~HRIRSphere()=default;

        virtual bool IsLoaded()const=0;

        /** HRIR 长度（采样数） */
        virtual uint32 GetIRLength()const=0;

        /** 数据采样率（Hz） */
        virtual uint32 GetSampleRate()const=0;

        virtual uint32 GetVertexCount()const=0;
        virtual uint32 GetFaceCount()const=0;

        virtual SamplingMode GetSamplingMode()const=0;
        virtual void SetSamplingMode(SamplingMode mode)=0;

        /**
        * 采样指定方向的左右耳 HRIR
        * @param direction 方向（单位向量，不必精确归一）
        * @param left_hrir 输出左耳 HRIR（长度 >= GetIRLength()）
        * @param right_hrir 输出右耳 HRIR（长度 >= GetIRLength()）
        */
        virtual void Sample(const Vec3 &direction,float *left_hrir,float *right_hrir)const=0;
    };

    /**
    * .amir 格式实现
    */
    class HRIRSphereImpl final : public HRIRSphere
    {
        struct Vertex
        {
            Vec3 position;
            std::vector<float> left_ir;
            std::vector<float> right_ir;
            float left_delay=0.0f;
            float right_delay=0.0f;
        };

        struct Face
        {
            uint32 a,b,c;
        };

        struct Barycentric
        {
            float u,v,w;
        };

        static constexpr float kEpsilon=1e-5f;

    public:
        HRIRSphereImpl()=default;
        ~HRIRSphereImpl()override=default;

        /** 加载 .amir 文件（CMCore 文件流） */
        bool LoadFromFile(const char *utf8_path);

        bool IsLoaded()const override{return loaded;}

        uint32 GetIRLength()const override{return header.ir_length;}
        uint32 GetSampleRate()const override{return header.sample_rate;}

        uint32 GetVertexCount()const override{return (uint32)vertices.size();}
        uint32 GetFaceCount()const override{return (uint32)faces.size();}

        SamplingMode GetSamplingMode()const override{return sampling_mode;}
        void SetSamplingMode(SamplingMode mode)override{sampling_mode=mode;}

        void Sample(const Vec3 &direction,float *left_hrir,float *right_hrir)const override
        {
            if(sampling_mode==SamplingMode::Bilinear)
                SampleBilinear(direction,left_hrir,right_hrir);
            else
                SampleNearestNeighbor(direction,left_hrir,right_hrir);
        }

    private:
        /** 射线三角形相交（Möller-Trumbore），返回重心坐标 */
        bool RayTriangleIntersection(const Vec3 &origin,const Vec3 &dir,const Vec3 &v0,const Vec3 &v1,const Vec3 &v2,Barycentric &out)const;

        /** 线性查找方向所在面（替代 Amplitude 的 FaceBSPTree——采样非实时，线性可接受） */
        const Face *FindFace(const Vec3 &dir)const;

        const Vertex *GetClosestVertex(const Vec3 &position,const Face *face)const;

        void SampleBilinear(const Vec3 &direction,float *left_hrir,float *right_hrir)const;
        void SampleNearestNeighbor(const Vec3 &direction,float *left_hrir,float *right_hrir)const;

        struct Header
        {
            char magic[4];
            uint16 version=0;
            uint32 sample_rate=0;
            uint32 ir_length=0;
            uint32 vertex_count=0;
            uint32 index_count=0;
        } header;

        std::vector<Vertex> vertices;
        std::vector<Face> faces;

        SamplingMode sampling_mode=SamplingMode::NearestNeighbor;
        bool loaded=false;
    };
}//namespace hgl::audio
