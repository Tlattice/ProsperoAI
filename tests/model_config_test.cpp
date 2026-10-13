// SPDX-License-Identifier: GPL-3.0-or-later
#include "model_config.hpp"
#define PROSPERO_SETTINGS_PATH "runtime.cfg"
#include "runtime_settings.hpp"
#include <unistd.h>
#include <cassert>
#include <fstream>
int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string root = argv[1];
    assert(chdir(root.c_str()) == 0);
    {
        std::ofstream f("runtime.cfg");
        f << "version=2\nmodel=sd-turbo-fp16\nmodel_root=/mnt/usb0/models\n# retained "
             "comment\nfuture_setting=yes";
    }
    assert(prospero_settings::preference_extensions_unlocked() ==
           "model_root=/mnt/usb0/models\n# retained comment\nfuture_setting=yes\n");
    assert(prospero_settings::set({32768, 0, true}));
    std::string configured;
    prospero::read_model_config("runtime.cfg",
                                [&](auto key, auto value)
                                {
                                    if (key == "model_root")
                                        configured = value;
                                });
    assert(configured == "/mnt/usb0/models");
    auto p = prospero::image_parameters(root);
    assert(p.steps == 1 && p.cfg == 1 && p.seed == 42 && p.width == 512);
    {
        std::ofstream f(root + "/prospero-params.cfg");
        f << "steps=200\ncfg=nan\nseed=-1\nwidth=770\nheight=0\nclip_skip=2\nprediction=v\nsampler="
             "euler_a\nnegative_prompt=lowres, bad anatomy\nunknown=4\n";
    }
    p = prospero::image_parameters(root);
    assert(p.steps == 100 && p.cfg == 1 && p.seed == -1 && p.width == 768 && p.height == 64);
    assert(p.clip_skip == 2 && p.prediction == "v" && p.sampler == "euler_a" &&
           p.negative_prompt == "lowres, bad anatomy");
    {
        std::ofstream f(root + "/prospero-params.cfg");
        f << "steps=4\ncfg=7.5\nsteps=garbage\nprediction=garbage\n";
    }
    p = prospero::image_parameters(root);
    assert(p.steps == 4 && p.cfg == 7.5f && p.prediction == "eps");
}
