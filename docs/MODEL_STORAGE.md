# Model storage and image parameters

Add an absolute path to `/data/prosperoai/prosperoai.cfg`, for example:

```ini
model_root=/mnt/usb0/ProsperoAI/models
```

Create the folder first. At startup the app selects it only when it is a readable
existing directory; otherwise it uses the internal models folder. The Models page
and startup log show the selected root. Both native preferences and HTTP settings rewrites preserve this line.
Restart after changing it or connecting a drive. This implementation uses one root;
it does not merge internal/external catalogs or switch roots during generation.
Older app-local models are adopted into internal shared storage before selection.

Startup logs probe `/mnt/usb0` through `/mnt/usb7` and `/mnt/ext0` with `opendir`
and report errno. On the tested console, `/mnt/usb0` is an exFAT mount and app access is verified.
Other slots remain candidate paths. Check your console's filesystem before copying
large models. Physical unplug/start behavior still requires a console test.

Downloads target the selected root; bundle staging stays on that volume. Free space
is checked before transfer (`statvfs` on the host, libkernel `_fstatfs` on console).
The compatibility archive's raw `statfs` syscall is unsafe inside an app. Checks
conservatively require the full download size, including
when resuming. Network interruptions preserve resumable data. Cancellation and storage
write failures attempt partial-file cleanup; a removed drive cannot be cleaned while
it is inaccessible. Reconnect it and retry/cancel to remove remaining staging files.

Custom image folders directly below the root are discovered through `model.json`.
Git LFS pointer files are not model weights: download the actual binaries before
copying a cloned model. For the tested Waifu Diffusers bundle, the loader reads
`unet/diffusion_pytorch_model.safetensors`, `text_encoder/model.safetensors` and
`vae/diffusion_pytorch_model.safetensors`; its FP16 source files were installed under
those filenames.
The runtime validates their contents when loading; discovery alone does not prove a
bundle is supported. Curated bundles retain their existing completeness checks.

Put image defaults in `<model-folder>/prospero-params.cfg`. It is read every generation;
`model.json` is never changed. For a conventional SD 1.x model, an example is:

```ini
steps=24
cfg=7.0
seed=-1
width=512
height=512
sampler=euler_a
prediction=eps
clip_skip=2
negative_prompt=lowres, bad anatomy, worst quality
```

Missing files preserve the existing one-step, CFG 1, seed 42, 512×512 behavior.
Steps clamp to 1–100, CFG to 0–20, seed to at least -1, and clip skip to -1–12.
Dimensions clamp to 64–1024 and round down to multiples of 64. Invalid numbers,
unknown keys and unsupported enum values are ignored. Supported samplers are
`euler`, `euler_a`, `heun`, `dpm++2m` and `lcm`. Prediction accepts `eps` and `v`;
changing it reloads the cached context. Creating or removing a parameter file also
reloads the context because the pinned loader validates the bundle directory
timestamp; editing an existing file applies per image. Avoid changing bundle files
during generation. Applied
values go to the system log and, when enabled, the debug trace.

The downloader creates defaults for the current curated SD-Turbo family. Additional
family detection and VAE precision controls are not implemented. SD-Turbo was verified from USB with no parameter file: output matched the internal
baseline byte-for-byte; cold requests took about 62 s from USB versus 30 s internally,
and warm requests took about 1.2 s on both. A live `steps=4` edit produced progress
through 4/4 and a warm request of about 1.9 s. A valid 256×256 TGA and a live edit
back to 512×512 were also verified. Removing the file reloaded the context and
restored byte-identical default output. Waifu Diffusion was also verified from USB with 24 steps, CFG 7 and epsilon
prediction: about 75 s for a cold request and 12.5 s warm, producing a valid 512×512
image. These are smoke-test timings.
The app folder on this console is also on USB, so a physical unplug/start test
requires a separate internal app installation. Root fallback is covered by host tests.
