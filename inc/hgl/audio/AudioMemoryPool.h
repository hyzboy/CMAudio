#pragma once

#include<hgl/log/Log.h>
#include<hgl/type/String.h>

namespace hgl
{
    namespace audio
    {
        /**
         * 音频内存池模板
         * 用于减少频繁的内存分配/释放操作
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
                if(buffer)
                {
                    delete[] buffer;
                    buffer = nullptr;
                }
            }
            
            /**
             * 禁止拷贝构造
             */
            AudioMemoryPool(const AudioMemoryPool&) = delete;
            AudioMemoryPool& operator=(const AudioMemoryPool&) = delete;
            
            /**
             * 确保缓冲区有足够大小
             * @param requiredSize 需要的大小(元素数量)
             * @param growthFactor 增长因子 (默认1.5倍)
             */
            void Ensure(uint requiredSize, float growthFactor = 1.5f)
            {
                if(requiredSize <= bufferSize)
                    return;  // 当前缓冲区足够大
                
                // 计算新大小 (使用增长因子)
                uint newSize = requiredSize + static_cast<uint>(requiredSize * (growthFactor - 1.0f));
                
                LogInfo(poolName + OS_TEXT(" expanding from ") + 
                        OSString::numberOf((int)bufferSize) +
                        OS_TEXT(" to ") + OSString::numberOf((int)newSize) + 
                        OS_TEXT(" elements"));
                
                // 分配新缓冲区
                T* newBuffer = new T[newSize];
                
                // 如果有旧数据，复制过来
                if(buffer && bufferSize > 0)
                {
                    memcpy(newBuffer, buffer, bufferSize * sizeof(T));
                    delete[] buffer;
                }
                
                buffer = newBuffer;
                bufferSize = newSize;
            }
            
            /**
             * 预分配缓冲区(基于估算大小的倍数)
             * @param estimatedSize 估算大小(元素数量)
             * @param multiplier 倍数 (默认2倍)
             */
            void Preallocate(uint estimatedSize, float multiplier = 2.0f)
            {
                uint targetSize = static_cast<uint>(estimatedSize * multiplier);
                
                if(targetSize > bufferSize)
                {
                    LogInfo(poolName + OS_TEXT(" preallocating ") + 
                            OSString::numberOf((int)targetSize) + 
                            OS_TEXT(" elements (") + 
                            OSString::numberOf(multiplier) + 
                            OS_TEXT("x estimated)"));
                    
                    if(buffer)
                        delete[] buffer;
                    
                    buffer = new T[targetSize];
                    bufferSize = targetSize;
                }
            }
            
            /**
             * 确保缓冲区大小(可选预估大小用于预分配)
             * @param requiredSize 需要的大小(元素数量)
             * @param estimatedSize 预估大小(元素数量),如果提供则预分配2倍大小
             */
            void EnsureWithEstimate(uint requiredSize, uint estimatedSize = 0)
            {
                // 如果提供了预估大小，使用2倍预估大小
                uint targetSize = estimatedSize > 0 ? (estimatedSize * 2) : requiredSize;
                
                // 如果targetSize还是小于requiredSize，使用1.5倍requiredSize
                if(targetSize < requiredSize)
                    targetSize = requiredSize + (requiredSize >> 1);
                
                if(targetSize <= bufferSize)
                    return;  // 当前缓冲区足够大
                
                LogInfo(poolName + OS_TEXT(" expanding from ") + 
                        OSString::numberOf((int)bufferSize) +
                        OS_TEXT(" to ") + OSString::numberOf((int)targetSize) + 
                        OS_TEXT(" elements"));
                
                if(buffer)
                    delete[] buffer;
                
                buffer = new T[targetSize];
                bufferSize = targetSize;
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
             * 重置缓冲区(清零但不释放内存)
             * @param count 要清零的元素数量(默认全部)
             */
            void Clear(uint count = 0)
            {
                if(buffer && bufferSize > 0)
                {
                    uint clearCount = (count == 0 || count > bufferSize) ? bufferSize : count;
                    memset(buffer, 0, clearCount * sizeof(T));
                }
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
    }
}
