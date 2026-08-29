// AudioMixerScene TOML Configuration Parser
// Simple TOML parser for AudioMixerScene multi-source mixing configuration
#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <hgl/audio/AudioMixerScene.h>
#include <hgl/type/StringList.h>

namespace hgl
{
    namespace audio
    {
        /**
         * AudioMixerScene configuration loaded from TOML file
         */
        struct SceneConfigData
        {
            struct OutputConfig
            {
                AudioDataInfo info;   // channels/bits_per_sample/is_float/sample_rate
                OutputConfig()
                {
                    info.channels = 1;
                    info.bits_per_sample = 16;
                    info.is_float = false;
                    info.sample_rate = 44100;
                }
            } output;

            struct SourceConfig
            {
                std::string name;
                std::string wavFile;
                uint min_count = 1;
                uint max_count = 1;
                float min_interval = 0.0f;
                float max_interval = 0.0f;
                float min_volume = 1.0f;
                float max_volume = 1.0f;
                float min_pitch = 1.0f;
                float max_pitch = 1.0f;
            };

            std::vector<SourceConfig> sources;
            float duration = 10.0f;
        };

        /**
         * Simple TOML configuration parser for AudioMixerScene
         */
        class AudioMixerSceneConfig
        {
        private:
            static std::string Trim(const std::string& str)
            {
                size_t first = str.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) return "";
                size_t last = str.find_last_not_of(" \t\r\n");
                return str.substr(first, last - first + 1);
            }

            static std::string UnquoteString(const std::string& str)
            {
                std::string s = Trim(str);
                if (s.length() >= 2 && s[0] == '"' && s[s.length()-1] == '"')
                    return s.substr(1, s.length() - 2);
                return s;
            }

            static int ParseInt(const std::string& value)
            {
                return std::stoi(Trim(value));
            }

            static float ParseFloat(const std::string& value)
            {
                return std::stof(Trim(value));
            }

        public:
            /**
             * Load AudioMixerScene configuration from TOML file
             * @param filename Path to TOML configuration file
             * @param config Output configuration data
             * @return true if successful
             */
            static bool Load(const char* filename, SceneConfigData& config)
            {
                std::ifstream file(filename);
                if (!file.is_open())
                {
                    std::cerr << "Error: Cannot open " << filename << std::endl;
                    return false;
                }

                std::string line;
                std::string currentSection;
                SceneConfigData::SourceConfig currentSource;
                bool inSourceSection = false;

                while (std::getline(file, line))
                {
                    line = Trim(line);

                    // Skip empty lines and comments
                    if (line.empty() || line[0] == '#')
                        continue;

                    // Section header
                    if (line[0] == '[' && line[line.length()-1] == ']')
                    {
                        // Save previous source if we were in a source section
                        if (inSourceSection)
                        {
                            config.sources.push_back(currentSource);
                            currentSource = SceneConfigData::SourceConfig();
                        }

                        currentSection = line.substr(1, line.length() - 2);
                        inSourceSection = (currentSection.find("source.") == 0);

                        if (inSourceSection)
                        {
                            // Extract source name from section like [source.car]
                            currentSource.name = currentSection.substr(7); // Skip "source."
                        }
                        continue;
                    }

                    // Key-value pair
                    size_t eqPos = line.find('=');
                    if (eqPos == std::string::npos)
                        continue;

                    std::string key = Trim(line.substr(0, eqPos));
                    std::string value = Trim(line.substr(eqPos + 1));

                    // Parse based on current section
                    if (currentSection == "output")
                    {
                        if (key == "sample_rate")
                            config.output.info.sample_rate = ParseInt(value);
                        else if (key == "format")
                        {
                            std::string fmt = UnquoteString(value);
                            if (fmt == "MONO16")
                            {
                                config.output.info.channels = 1;
                                config.output.info.bits_per_sample = 16;
                                config.output.info.is_float = false;
                            }
                            else if (fmt == "MONO8")
                            {
                                config.output.info.channels = 1;
                                config.output.info.bits_per_sample = 8;
                                config.output.info.is_float = false;
                            }
                        }
                    }
                    else if (currentSection == "scene")
                    {
                        if (key == "duration")
                            config.duration = ParseFloat(value);
                    }
                    else if (inSourceSection)
                    {
                        if (key == "wav_file")
                            currentSource.wavFile = UnquoteString(value);
                        else if (key == "min_count")
                            currentSource.min_count = ParseInt(value);
                        else if (key == "max_count")
                            currentSource.max_count = ParseInt(value);
                        else if (key == "min_interval")
                            currentSource.min_interval = ParseFloat(value);
                        else if (key == "max_interval")
                            currentSource.max_interval = ParseFloat(value);
                        else if (key == "min_volume")
                            currentSource.min_volume = ParseFloat(value);
                        else if (key == "max_volume")
                            currentSource.max_volume = ParseFloat(value);
                        else if (key == "min_pitch")
                            currentSource.min_pitch = ParseFloat(value);
                        else if (key == "max_pitch")
                            currentSource.max_pitch = ParseFloat(value);
                    }
                }

                // Save last source
                if (inSourceSection)
                {
                    config.sources.push_back(currentSource);
                }

                file.close();
                return true;
            }
        };
    } // namespace audio
} // namespace hgl
