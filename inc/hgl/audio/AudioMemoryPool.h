#pragma once

#include<cstring>
#include<hgl/log/Log.h>
#include<hgl/type/String.h>

namespace hgl::audio
{
    /**
        * 音频内存池模板
        * 用于减少频繁的内存分配/释放操作
        *
        * 说明: 音频混音/重采样场景中缓冲区是"整体重写"的（先清零再填充），
        * 因此扩容时不保留旧数据（不做 memcpy），避免无意义的拷贝开销。
        *
        * @tparam T 元素类型 (如 float, char等)
        */
    template<typename T>
    class AudioMemoryPool
    {
    private:

        T* buffer;              ///< 缓冲区指针
        uint bufferSize;        ///< 缓冲区大小(元素数量)
        OSString poolName;      ///< 内存池名称(用于日志)

    public:
        /**
            * 构造函数
            * @param name 内存池名称(用于日志识别)
            */
        AudioMemoryPool(const OSString& name = OS_TEXT("Pool"))
            : buffer(nullptr), bufferSize(0), poolName(name)
        {
        }

        /**
            * 析构函数 - 自动释放内存
            */
        ~AudioMemoryPool()
        {
            Release();
        }

        /**
            * 禁止拷贝构造
            */
        AudioMemoryPool(const AudioMemoryPool&) = delete;
        AudioMemoryPool& operator=(const AudioMemoryPool&) = delete;

        /**
            * 确保缓冲区有足够大小(元素数量)
            * 不足时按 1.5 倍增长策略扩容（不保留旧数据）
            * @param requiredSize 需要的元素数量
            */
        void Ensure(uint requiredSize)
        {
            if(requiredSize <= bufferSize)
                return;  // 当前缓冲区足够大

            // 1.5 倍增长，且至少满足 requiredSize
            uint newSize = bufferSize + (bufferSize >> 1);   // bufferSize * 1.5
            if(newSize < requiredSize)
                newSize = requiredSize;

            GLogInfo(poolName + OS_TEXT(" expanding from ") +
                    OSString::numberOf(bufferSize) +
                    OS_TEXT(" to ") + OSString::numberOf(newSize) +
                    OS_TEXT(" elements"));

            delete[] buffer;

            buffer = new T[newSize];
            bufferSize = newSize;
        }

        /**
            * 获取缓冲区指针
            * @return 缓冲区指针
            */
        T* Get() { return buffer; }

        /**
            * 获取缓冲区指针(const版本)
            * @return 缓冲区指针
            */
        const T* Get() const { return buffer; }

        /**
            * 获取当前缓冲区大小(元素数量)
            * @return 缓冲区大小
            */
        uint GetSize() const { return bufferSize; }

        /**
            * 清零缓冲区(不释放内存)
            */
        void Clear()
        {
            if(buffer && bufferSize > 0)
                memset(buffer, 0, bufferSize * sizeof(T));
        }

        /**
            * 释放缓冲区
            */
        void Release()
        {
            if(buffer)
            {
                delete[] buffer;
                buffer = nullptr;
                bufferSize = 0;
            }
        }
    };
}// namespace hgl::audio
