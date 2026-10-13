// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
namespace prospero
{
inline std::string config_trim(std::string value)
{
    auto first = value.find_first_not_of(" \t\r\n");
    return first == std::string::npos
               ? ""
               : value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
template <class Visitor> inline void read_model_config(const std::string &path, Visitor visit)
{
    FILE *file = std::fopen(path.c_str(), "r");
    if (!file)
        return;
    char line[4096];
    while (std::fgets(line, sizeof(line), file))
    {
        std::string text = config_trim(line);
        // Ignore overlong lines rather than parsing their trailing fragments.
        if (text.size() && text.back() != '\n' && !std::strchr(line, '\n') && !std::feof(file))
        {
            int c;
            while ((c = std::fgetc(file)) != '\n' && c != EOF)
            {
            }
            continue;
        }
        auto at = text.find('=');
        if (text.empty() || text[0] == '#' || text[0] == ';' || at == std::string::npos)
            continue;
        visit(config_trim(text.substr(0, at)), config_trim(text.substr(at + 1)));
    }
    std::fclose(file);
}
inline std::string select_model_root(const std::string &settings, const std::string &fallback)
{
    std::string requested;
    read_model_config(settings,
                      [&](const std::string &key, const std::string &value)
                      {
                          if (key == "model_root")
                              requested = value;
                      });
    struct stat info
    {
    };
    if (requested.empty() || requested[0] != '/' || stat(requested.c_str(), &info) != 0 ||
        !S_ISDIR(info.st_mode))
        return fallback;
    DIR *directory = opendir(requested.c_str());
    if (!directory)
        return fallback;
    closedir(directory);
    while (requested.size() > 1 && requested.back() == '/')
        requested.pop_back();
    return requested;
}
struct ImageParameters
{
    int steps = 1, width = 512, height = 512, clip_skip = -1;
    float cfg = 1;
    std::int64_t seed = 42;
    std::string sampler, prediction = "eps", negative_prompt;
};
inline ImageParameters image_parameters(const std::string &root)
{
    ImageParameters result;
    read_model_config(root + "/prospero-params.cfg",
                      [&](const std::string &key, const std::string &value)
                      {
                          if (key == "negative_prompt")
                          {
                              result.negative_prompt = value;
                              return;
                          }
                          if (key == "sampler")
                          {
                              if (value == "euler" || value == "euler_a" || value == "heun" ||
                                  value == "dpm++2m" || value == "lcm")
                                  result.sampler = value;
                              return;
                          }
                          if (key == "prediction")
                          {
                              if (value == "eps" || value == "v")
                                  result.prediction = value;
                              return;
                          }
                          if (value.empty())
                              return;
                          char *end = nullptr;
                          errno = 0;
                          if (key == "cfg")
                          {
                              double number = std::strtod(value.c_str(), &end);
                              if (!errno && !*end && std::isfinite(number))
                                  result.cfg = std::clamp(number, 0.0, 20.0);
                              return;
                          }
                          auto number = std::strtoll(value.c_str(), &end, 10);
                          if (errno || *end)
                              return;
                          if (key == "steps")
                              result.steps = std::clamp(number, 1LL, 100LL);
                          else if (key == "seed")
                              result.seed = std::max(number, -1LL);
                          else if (key == "clip_skip")
                              result.clip_skip = std::clamp(number, -1LL, 12LL);
                          else if (key == "width")
                              result.width = (std::clamp(number, 64LL, 1024LL) / 64) * 64;
                          else if (key == "height")
                              result.height = (std::clamp(number, 64LL, 1024LL) / 64) * 64;
                      });
    return result;
}
} // namespace prospero
