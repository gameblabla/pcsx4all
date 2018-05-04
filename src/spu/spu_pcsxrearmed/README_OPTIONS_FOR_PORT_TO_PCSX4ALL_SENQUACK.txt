Until we add a -h switch to show help, I'll refer people to this
TXT file listing the various options of the spu_pcsxrearmed SPU
plugin I ported from PCSX ReARMed (credit goes to Notaz), which is
a heavily modified version of the P.E.O.P.S dfsound plugin by Pete.
-senquack Apr 24 2016

**********************************
- Optional settings you can try: -
**********************************
-nosyncaudio    (Emu won't wait and instead drop samples if output buffer full.)
-noforcedxaupdates (Don't use new feature that issues more frequent CD read
                    interrupts to keep XA audio buffer full)
-interpolation none,simple,gaussian,cubic  (upsamples audio, using more CPU)
-reverb         (enables reverb)
-xapitch	    (enable support for XA music/sfx speed pitch changes)
-notempo        (uses slower, more-compatible SPU cycle estimations, which
                 Pandora/Pyra/Android PCSX-ReARMed builds use as default.
                 Helps Final Fantasy games and Valkyrie Profile compatibility,
                 perhaps others, but can cause more audio stuttering.)
-nofixedupdates (don't output a fixed number of samples per frame.. I haven't
				 yet played with this much, but it is a configurable option in
				 PCSX-ReARMed for a reason, I guess)
-threaded_spu   (enable doing some sound processing in a thread. Almost
                 certainly won't help us in any way, but I did add command-line
                 switch to enable using Notaz's thread code. The Pandora builds
                 run an optimized version of the thread on their DSP chip.)
-volume 0..1024  (1024 is max volume, 0 will mute sound but keep the SPU plugin
                  running for max compatibility & allowing -syncaudio option
                  to have an effect. Use -silent flag to disable SPU plugin.)

**************
- REVISIONS: -
**************
Dec 11 2016:
  * Adopted as sole SPU plugin of this emu port, all others removed.
    Cleaned up sdl.c and removed old mutex cruft.

May 3  2016:
  * XA audio buffer was never filling and causing music/speech dropouts
    on slower devices because of two reasons:
    1.) Original FeedXA() was not properly detecting available unused space
        in buffer.
    2.) CD read interrupts (CDREAD_INT) are issued at a fixed rate that
        does not take into account slow emulation or used size of buffer.
    3.) General audio dropouts being caused by too-infrequent calls to
        SPU_async() in psxcounters.cpp. The original PCSX_ReARMed emu
        would only call it once per frame, but this is too seldom for slow
        devices. It is now called twice per frame, and this might be made
        a configurable setting in the future.

    These issues have now been fixed and XA audio is greatly improved.
    There is a chance that the new behavior could cause some incompatibilities,
    and you can try the new -noforcedxaupdates option to restore the
    original behavior.

Apr 25 2016:
  * Audio syncing is now on by default. Use -nosyncaudio to disable.
  * New option '-use_old_audio_mutex' enables old method of syncing audio,
    using mutex and condition variable). Default is now to use non-locking
    atomic shared var to syncronize SDL audio thread with emu.

