#include<hgl/audio/Ambisonics/AmbisonicDecoder.h>

#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include<cmath>
#include<lebedev/lebedev_quadrature.hpp>
#include<cstdio>

namespace hgl::audio
{
    // ===== 解码系数表（源自 Amplitude Audio SDK / Ambisonic 解码器常数）=====

    // Stereo 解码系数（2 声道，1 阶）
    constexpr float kDecoderCoefficientStereo[2][16]={
        { 0.5f,0.5f/3.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f },
        { 0.5f,-0.5f/3.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f }
    };

    // 5.1 一阶
    constexpr float kDecoderCoefficientFirst_5_1[6][4]={
        { 0.300520f,0.135000f,0.000000f,0.120000f },{ 0.300520f,-0.135000f,0.000000f,0.120000f },
        { 0.332340f,0.138333f,0.000000f,-0.110000f },{ 0.332340f,-0.138333f,0.000000f,-0.110000f },
        { 0.141421f,0.000000f,0.000000f,0.053333f },{ 0.500000f,0.000000f,0.000000f,0.000000f }
    };

    // 5.1 二阶
    constexpr float kDecoderCoefficientSecond_5_1[6][9]={
        { 0.286378f,0.103333f,-0.000000f,0.106667f,0.028868f,0.000000f,0.000000f,0.000000f,0.019630f },
        { 0.286378f,-0.103333f,-0.000000f,0.106667f,-0.028868f,0.000000f,0.000000f,-0.000000f,0.019630f },
        { 0.449013f,0.093333f,-0.000000f,-0.111667f,0.018475f,-0.000000f,-0.000000f,0.000000f,-0.018475f },
        { 0.449013f,-0.093333f,-0.000000f,-0.111667f,-0.018475f,-0.000000f,-0.000000f,0.000000f,-0.018475f },
        { 0.060104f,0.000000f,0.000000f,0.013333f,0.000000f,0.000000f,0.000000f,0.000000f,0.010392f },
        { 0.500000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f }
    };

    // 5.1 三阶
    constexpr float kDecoderCoefficientThird_5_1[6][16]={
        { 0.219203f,0.095000f,0.000000f,0.103333f,0.042724f,0.000000f,0.000000f,0.000000f,0.001155f,0.010842f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,-0.004518f },
        { 0.219203f,-0.095000f,0.000000f,0.103333f,-0.042724f,0.000000f,0.000000f,0.000000f,0.001155f,-0.010842f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,-0.004518f },
        { 0.417193f,0.128333f,0.000000f,-0.111667f,0.004619f,0.000000f,0.000000f,0.000000f,-0.005774f,-0.011746f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.004518f },
        { 0.417193f,-0.128333f,0.000000f,-0.111667f,-0.004619f,0.000000f,0.000000f,0.000000f,-0.005774f,0.011746f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.004518f },
        { 0.095459f,0.000000f,0.000000f,0.088333f,0.000000f,0.000000f,0.000000f,0.000000f,0.049652f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.018974f },
        { 0.500000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f }
    };

    // 7.1 一阶
    constexpr float kDecoderCoefficientFirst_7_1[8][4]={
        { 0.303082f,0.095958f,0.000000f,0.114243f },{ 0.303082f,-0.095958f,0.000000f,0.114243f },
        { 0.300098f,0.124767f,0.000000f,-0.017447f },{ 0.300098f,-0.124767f,0.000000f,-0.017447f },
        { 0.259458f,0.053266f,0.000000f,-0.117329f },{ 0.259458f,-0.053266f,0.000000f,-0.117329f },
        { 0.066262f,0.000000f,0.000000f,0.031737f },{ 0.500000f,0.000000f,0.000000f,0.000000f }
    };

    // 7.1 二阶
    constexpr float kDecoderCoefficientSecond_7_1[8][9]={
        { 0.268964f,0.090325f,0.000000f,0.111024f,0.044867f,0.000000f,0.000000f,0.000000f,0.015736f },
        { 0.268964f,-0.090325f,0.000000f,0.111024f,-0.044867f,-0.000000f,0.000000f,0.000000f,0.015736f },
        { 0.229483f,0.136694f,0.000000f,-0.018120f,-0.020953f,0.000000f,0.000000f,0.000000f,-0.049001f },
        { 0.229483f,-0.136694f,0.000000f,-0.018120f,0.020953f,-0.000000f,0.000000f,0.000000f,-0.049001f },
        { 0.216456f,0.042012f,0.000000f,-0.116220f,-0.038878f,0.000000f,0.000000f,0.000000f,0.032005f },
        { 0.216456f,-0.042012f,0.000000f,-0.116220f,0.038878f,-0.000000f,0.000000f,0.000000f,0.032005f },
        { 0.058222f,0.000000f,0.000000f,0.048933f,0.000000f,0.000000f,0.000000f,0.000000f,0.025293f },
        { 0.500000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f }
    };

    // 7.1 三阶
    constexpr float kDecoderCoefficientThird_7_1[8][16]={
        { 0.238475f,0.085873f,0.000000f,0.114877f,0.054573f,0.000000f,0.000000f,0.000000f,0.015163f,0.006254f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,-0.006185f },
        { 0.238475f,-0.085873f,0.000000f,0.114877f,-0.054573f,-0.000000f,0.000000f,0.000000f,0.015163f,-0.006254f,-0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,-0.006185f },
        { 0.214882f,0.124042f,0.000000f,-0.017580f,-0.018064f,0.000000f,0.000000f,0.000000f,-0.060255f,-0.011908f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.008159f },
        { 0.214882f,-0.124042f,0.000000f,-0.017580f,0.018064f,-0.000000f,0.000000f,0.000000f,-0.060255f,0.011908f,-0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.008159f },
        { 0.197904f,0.043357f,0.000000f,-0.115673f,-0.048364f,0.000000f,0.000000f,0.000000f,0.034129f,0.017198f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,-0.001868f },
        { 0.197904f,-0.043357f,0.000000f,-0.115673f,0.048364f,-0.000000f,0.000000f,0.000000f,0.034129f,-0.017198f,-0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,-0.001868f },
        { 0.077144f,0.000000f,0.000000f,0.045620f,0.000000f,0.000000f,0.000000f,0.000000f,0.030548f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.025329f },
        { 0.500000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f }
    };

    void AmbisonicDecoder::SetUpSpeakers(SpeakersPreset setUp,uint32 speaker_count)
    {
        speakersPreset=setUp;
        speakers.clear();
        isLoaded=false;

        switch(speakersPreset)
        {
        case SpeakersPreset::Custom:
            speakerCount=speaker_count;
            speakers.resize(speakerCount);

            for(uint32 i=0;i<speakerCount;i++)
                speakers[i].Configure(order,is3D);
            break;

        case SpeakersPreset::Mono:
            speakerCount=1;
            speakers.resize(1);
            speakers[0].Configure(order,is3D);
            speakers[0].SetPosition(SphericalPosition(0.0f,0.0f,1.0f));
            break;

        case SpeakersPreset::Stereo:
            speakerCount=2;
            speakers.resize(2);
            speakers[0].Configure(order,is3D);
            speakers[0].SetPosition(SphericalPosition(+30.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[1].Configure(order,is3D);
            speakers[1].SetPosition(SphericalPosition(-30.0f*Amb_DegToRad,0.0f,1.0f));
            break;

        case SpeakersPreset::Surround_5_1:
            speakerCount=6;
            speakers.resize(6);
            speakers[0].Configure(order,is3D);
            speakers[0].SetPosition(SphericalPosition(+30.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[1].Configure(order,is3D);
            speakers[1].SetPosition(SphericalPosition(-30.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[2].Configure(order,is3D);
            speakers[2].SetPosition(SphericalPosition(+110.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[3].Configure(order,is3D);
            speakers[3].SetPosition(SphericalPosition(-110.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[4].Configure(order,is3D);
            speakers[4].SetPosition(SphericalPosition(0.0f,0.0f,1.0f));
            // LFE
            speakers[5].Configure(order,is3D);
            speakers[5].SetPosition(SphericalPosition(0.0f,0.0f,0.0f));
            break;

        case SpeakersPreset::Surround_7_1:
            speakerCount=8;
            speakers.resize(8);
            speakers[0].Configure(order,is3D);
            speakers[0].SetPosition(SphericalPosition(+30.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[1].Configure(order,is3D);
            speakers[1].SetPosition(SphericalPosition(-30.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[2].Configure(order,is3D);
            speakers[2].SetPosition(SphericalPosition(+110.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[3].Configure(order,is3D);
            speakers[3].SetPosition(SphericalPosition(-110.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[4].Configure(order,is3D);
            speakers[4].SetPosition(SphericalPosition(+145.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[5].Configure(order,is3D);
            speakers[5].SetPosition(SphericalPosition(-145.0f*Amb_DegToRad,0.0f,1.0f));
            speakers[6].Configure(order,is3D);
            speakers[6].SetPosition(SphericalPosition(0.0f,0.0f,1.0f));
            // LFE
            speakers[7].Configure(order,is3D);
            speakers[7].SetPosition(SphericalPosition(0.0f,0.0f,0.0f));
            break;

        case SpeakersPreset::CubePoints:
            {
                speakerCount=8;
                speakers.resize(8);
                SphericalPosition position(0.0f,+35.2f*Amb_DegToRad,1.0f);

                for(uint32 i=0;i<4;i++)
                {
                    position.SetAzimuth(-(static_cast<float>(i)*360.f/4.0f+45.f)*Amb_DegToRad);
                    speakers[i].Configure(order,is3D);
                    speakers[i].SetPosition(position);
                }

                position.SetElevation(-35.2f*Amb_DegToRad);

                for(uint32 i=4;i<8;i++)
                {
                    position.SetAzimuth(-(static_cast<float>(i-4)*360.f/4.0f+45.f)*Amb_DegToRad);
                    speakers[i].Configure(order,is3D);
                    speakers[i].SetPosition(position);
                }
                break;
            }

        case SpeakersPreset::DodecahedronFaces:
            {
                speakerCount=12;
                speakers.resize(12);
                SphericalPosition position(0.0f,0.0f,1.0f);

                for(uint32 i=0;i<12;i++)
                {
                    position.SetAzimuth(-(i*360.f/12)*Amb_DegToRad);
                    speakers[i].Configure(order,is3D);
                    speakers[i].SetPosition(position);
                }
                break;
            }

        case SpeakersPreset::LebedevGridOrder26:
            {
                speakerCount=26;
                speakers.resize(26);

                const auto quad_points=lebedev::generate_quadrature_points(lebedev::QuadratureOrder::order_26);

                const auto &xAxis=std::get<0>(quad_points);
                const auto &yAxis=std::get<1>(quad_points);
                const auto &zAxis=std::get<2>(quad_points);

                for(uint32 i=0;i<26;i++)
                {
                    speakers[i].Configure(order,is3D);
                    speakers[i].SetPosition(SphericalPosition::ForHRTF(Vec3{
                        static_cast<float>(xAxis.at(i)),static_cast<float>(yAxis.at(i)),static_cast<float>(zAxis.at(i)) }));
                }
                break;
            }

        default:
            break;
        }

        const float speakerGain=1.0f/static_cast<float>(speakerCount);

        for(uint32 i=0;i<speakerCount;i++)
            speakers[i].SetGain(speakerGain);
    }

    void AmbisonicDecoder::DetectSpeakersPreset()
    {
        if(speakersPreset!=SpeakersPreset::Custom)
            return;

        uint32 speakerMatchCount=0;

        constexpr float azimuthStereo[2]={ 30.0f*Amb_DegToRad,-30.0f*Amb_DegToRad };
        constexpr float azimuthSurround51[6]={ 30.0f*Amb_DegToRad,-30.0f*Amb_DegToRad,110.0f*Amb_DegToRad,-110.0f*Amb_DegToRad,0.0f,0.0f };
        constexpr float azimuthSurround71[8]={ 30.0f*Amb_DegToRad,-30.0f*Amb_DegToRad,110.0f*Amb_DegToRad,-110.0f*Amb_DegToRad,145.0f*Amb_DegToRad,-145.0f*Amb_DegToRad,0.0f,0.0f };

        switch(speakerCount)
        {
        case 1:
            speakersPreset=SpeakersPreset::Mono;
            break;

        case 2:
            for(uint32 i=0;i<2;i++)
            {
                const SphericalPosition position=speakers[i].GetPosition();

                if(position.GetElevation()==0.0f&&std::abs(position.GetAzimuth()-azimuthStereo[i])<Amb_Epsilon)
                    speakerMatchCount++;
            }

            if(speakerMatchCount==2)
                speakersPreset=SpeakersPreset::Stereo;
            break;

        case 6:
            for(uint32 i=0;i<6;i++)
            {
                const SphericalPosition position=speakers[i].GetPosition();

                if(position.GetElevation()==0.0f&&std::abs(position.GetAzimuth()-azimuthSurround51[i])<Amb_Epsilon)
                    speakerMatchCount++;
            }

            if(speakerMatchCount==6)
                speakersPreset=SpeakersPreset::Surround_5_1;
            break;

        case 8:
            for(uint32 i=0;i<8;i++)
            {
                const SphericalPosition position=speakers[i].GetPosition();

                if(position.GetElevation()==0.0f&&std::abs(position.GetAzimuth()-azimuthSurround71[i])<Amb_Epsilon)
                    speakerMatchCount++;
            }

            if(speakerMatchCount==8)
                speakersPreset=SpeakersPreset::Surround_7_1;
            break;

        default:
            break;
        }
    }

    void AmbisonicDecoder::LoadDecoderPreset()
    {
        if(speakersPreset==SpeakersPreset::Custom)
            return;

        if(speakerCount==0||speakers.empty())
            return;

        const bool isStereo=(speakersPreset==SpeakersPreset::Stereo);
        const bool is5_1=(speakersPreset==SpeakersPreset::Surround_5_1);
        const bool is7_1=(speakersPreset==SpeakersPreset::Surround_7_1);

        if(isStereo)
        {
            for(uint32 s=0;s<speakerCount;s++)
                for(uint32 c=0;c<channelCount&&c<16;c++)
                    speakers[s].SetCoefficient(c,kDecoderCoefficientStereo[s][c]);
        }
        else if(is5_1)
        {
            if(order==1)
                for(uint32 s=0;s<speakerCount;s++)
                    for(uint32 c=0;c<channelCount&&c<4;c++)
                        speakers[s].SetCoefficient(c,kDecoderCoefficientFirst_5_1[s][c]);
            else if(order==2)
                for(uint32 s=0;s<speakerCount;s++)
                    for(uint32 c=0;c<channelCount&&c<9;c++)
                        speakers[s].SetCoefficient(c,kDecoderCoefficientSecond_5_1[s][c]);
            else if(order==3)
                for(uint32 s=0;s<speakerCount;s++)
                    for(uint32 c=0;c<channelCount&&c<16;c++)
                        speakers[s].SetCoefficient(c,kDecoderCoefficientThird_5_1[s][c]);
        }
        else if(is7_1)
        {
            if(order==1)
                for(uint32 s=0;s<speakerCount;s++)
                    for(uint32 c=0;c<channelCount&&c<4;c++)
                        speakers[s].SetCoefficient(c,kDecoderCoefficientFirst_7_1[s][c]);
            else if(order==2)
                for(uint32 s=0;s<speakerCount;s++)
                    for(uint32 c=0;c<channelCount&&c<9;c++)
                        speakers[s].SetCoefficient(c,kDecoderCoefficientSecond_7_1[s][c]);
            else if(order==3)
                for(uint32 s=0;s<speakerCount;s++)
                    for(uint32 c=0;c<channelCount&&c<16;c++)
                        speakers[s].SetCoefficient(c,kDecoderCoefficientThird_7_1[s][c]);
        }

        isLoaded=true;
    }
}//namespace hgl::audio
