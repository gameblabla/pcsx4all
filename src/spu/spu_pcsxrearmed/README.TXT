/******************************************************************************
  This is PCSX-ReARMed's SPU, adapted for use with the older PSX4ALL.
  At the time of this import, pcsx_rearmed was at r22 (2015-02-05).
  Original source (GPL license):
  http://notaz.gp2x.de/pcsx_rearmed.php
  https://github.com/notaz/pcsx_rearmed/tree/master/plugins/dfsound
  git://notaz.gp2x.de/~notaz/pcsx_rearmed.git
 
  It has numerous bug fixes by Notaz, so much thanks and credit go to him.
  Its roots go back to the original P.E.Op.S. DSound PSX SPU, some
  parts adapted by notaz from PCSX-Reloaded, but with an eye towards
  performance on limited devices. 
  I (senquack) have improved sound syncronization with emulator to fix
  dropouts/clicks, but this is an optional setting.

  adapted for use with older PCSX4ALL by senquack 
  https://github.com/senquack   dansilsby <AT> gmail (DOT) com

  P.E.Op.S. SPU plugin:
  (C) Pete Bernert and the P.E.Op.S. team
  (C) SPU2-X, gigaherz, Pcsx2 Development Team
  shalma
  notaz
  
******************************************************************************/

// GIT COMMIT LOG AT TIME OF CODE IMPORT FOLLOWS:
// ( https://github.com/notaz/pcsx_rearmed/tree/master/plugins/dfsound )

commit 8f2bb0cbd20c00475196d1efb72103b65e57bc66
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Apr 1 01:49:55 2016 +0300

    don't use __ARM_ARCH_7A__
    
    github issue #68

commit 1e0eac2348343c8a046f7a75d460f087556cf7f6
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Feb 6 01:26:44 2015 +0200

    libretro: always use fixed samples per frame

commit 0c1151feca09186ec61485436912a3e563c40b54
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Feb 2 04:13:30 2015 +0200

    spu: c64x: fix cache handling, enable l2

commit 7b2c4897f089ab6eee7bc2ce2873c2b11058e04e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Feb 2 03:51:40 2015 +0200

    spu: fix some thread safety issues
    
    reverb and stuff

commit 05c7cec77522f04857f655474574469a5e66661d
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Jan 11 03:35:32 2015 +0200

    spu: put reverb on the thread too
    
    and more stuff to work struct

commit 8f5f2dd5a70f47322614eda6f97304808447199c
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 10 03:51:44 2015 +0200

    some random improvements

commit c632283d5c48d7731ec5704c3d5eef54951fec20
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jan 12 03:00:12 2015 +0200

    spu: fix some skip accuracy issues

commit e541b8e06ff03a7fe7bd75b1bc0ad53f60b2d0de
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jan 5 23:50:33 2015 +0200

    spu: don't block on audio

commit 3bd31caf9e9f5ddab2bf4fbdb5a129f4972c45f3
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 10 01:55:24 2015 +0200

    spu: support multiple pending buffers
    
    for threaded implementation

commit de4a0279efefdd2e4595c8fc27f1564f4bff9341
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jan 5 04:27:03 2015 +0200

    spu: finish offload code to TI C64x DSP
    
    rather disappointing result so far, though :(

commit 5514a050f4e936f5c333fa1271b11bf5a6a9ea39
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Jan 4 01:43:23 2015 +0200

    spu: start offload code to TI C64x DSP

commit 63a4f6b6a3b0315590cd3009df2c92480ed2d98b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jan 2 04:13:50 2015 +0200

    spu: threaded implementation
    
    unlike original P.E.Op.S. implementation, the goal here is to carefully
    avoid any side effects caused by having the thread enabled

commit 9ad8abfa940cd5c13eb0653639ea86736b65a2c4
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jan 2 04:12:35 2015 +0200

    spu: get rid of bStop, clean up

commit 4ccd0fb2874dab39c9db8ab973d4db9b8d54fa0d
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Jan 1 19:35:16 2015 +0200

    spu: move more things to StartSound

commit 011755d7a88f7128d04a14136e9fd61870706dcf
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Dec 31 02:15:08 2014 +0200

    spu: reduce use of globals
    
    to be used by upcoming threaded code..

commit 215ff9e69c0b845f24e7a3aa9faeef06d9276145
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Dec 27 23:24:15 2014 +0200

    spu: handle stop better, split main func more

commit 003cfc63c69b3e900de768317354dfb80bbf6a55
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 3 02:22:14 2015 +0200

    mingw build fixes

commit bdd050c3ed792381df2e744fee5b7ee80b93fd68
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Dec 21 02:20:41 2014 +0200

    spu: fix first sample, don't play ahead

commit c4c66b22ae607aa8dc5e6983f7ab030ead01faae
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Dec 21 00:36:00 2014 +0200

    spu: avoid NULL pSpuIrq
    
    CTR sets 0 irq address, and savestates mess up

commit 3154bfab51566cbaa5bce3965d4c915bfb1b4f53
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Dec 19 03:33:20 2014 +0200

    spu: put globals into a structure
    
    - also clean up some unused stuff
    - put spu config into it's own structure and header

commit 54933c929d2e418e925ae823e3fa7607b2d5ac85
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Dec 18 23:47:06 2014 +0200

    spu: remove some hacks

commit 650adfd2da779ba8855623362c2900583e22931e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Dec 18 23:43:08 2014 +0200

    spu: rework synchronization

commit 2b30c1291db9d9801d51cf85f71f40fe54958898
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Dec 18 03:56:01 2014 +0200

    spu: add a schedule callback

commit e391d21cdbf06afd68d137537b7e82585ecffa1a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Tue Dec 9 04:11:36 2014 +0200

    spu: fix some fmod related breakage

commit 1f4e070a56a597038942d595b82476ca5dc249fd
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Apr 1 19:43:48 2013 +0300

    asm: fix incorrect offsets

commit 4ae83961a97c97cf6bf7e775cc08337b292b9853
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Apr 1 01:08:33 2013 +0300

    still adjusting asm for better as compatibility..

commit 5c6457c3a3e44b0f39be5221ad4a9d94a432d694
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Mar 25 02:14:54 2013 +0200

    asm: use a macro for functions

commit 706252fcdbe50e0be5779703d1e92fc849f9e93f
Author: twinaphex <autechre1024 <AT> hotmail (DOT) com>
Date:   Thu Mar 21 23:04:13 2013 +0100

    Don't compile in null sound driver for libretro and add
    Makefile.libretro

commit 16187bfb1518db23ade931cd7358ba973a585180
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Mar 16 04:14:24 2013 +0200

    spu: change sync again
    
    will hurt audio if game is way too slow, but perhaps improve
    sync situation a bit..

commit ee9afdbd4d9e0aee75dd9e17878c76f1c7236d04
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 5 04:05:56 2013 +0200

    spu: fix decode offset

commit f9fe492349dcb13ca16e1c809bd2bd25f4a73f10
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 5 04:04:35 2013 +0200

    spu: stop at address 0
    
    or should we? (Music 2000)

commit e34a4bd3b3eec3f95f5ce0cac1c4d47bf03a0409
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 5 00:55:59 2013 +0200

    spu: check buffer state less often

commit 7ba3e79c025dfd41ba88bcbe7b330a1a7fd68551
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Jan 3 02:00:44 2013 +0200

    spu: also return on decoder irqs

commit 1d753163fcfe8f75fad07c413ebbdb8cfe448f8b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jan 2 00:33:53 2013 +0200

    spu: fix decode buffers
    
    sync is still broken :(

commit b68bf5c238ef32ed93eb8935c5b0288da5b3ad67
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Dec 23 02:21:53 2012 +0200

    frontend: fix pulseaudio, detect it in configure script

commit 665f33e1e8ce2e40a7939a33075c3bce1c90790c
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Nov 30 02:53:25 2012 +0200

    improve ARM feature detection

commit 6f6fe96900374d8744473ce240279e66d1260191
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Nov 22 02:48:19 2012 +0200

    spu: fix saving bug
    
    fixup on load too

commit c668f24877c063db881fa8d1eeb8a6fbc649d5a0
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Nov 22 00:18:21 2012 +0200

    fix some more 32/64bit incompatibilities

commit 1b908f9e6e455d3eecf3b9a9236405c29764c0f8
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Nov 19 00:23:45 2012 +0200

    dfsound: add PulseAudio workaround

commit 07bd309f731eefa49208d77a35507a7b7448ffb6
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Nov 16 00:30:32 2012 +0200

    spu: stop at 0 volume

commit 8e1040b6a82ee1802fda83d4de84290a02656cc0
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Nov 5 03:11:29 2012 +0200

    sort out 32/64bit savestate compat issues

commit 07c13dfd11cbd45e96a5c21cd88ccd44601678ea
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jul 30 21:29:29 2012 +0300

    allow multiple sound drivers to be compiled

commit f05d6ca255c80170e4e5fc61cc48d87e013b8807
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jul 30 16:27:32 2012 +0300

    spu: some cleanups

commit c67af2ac1a8305c7377c7dda844257c5bc1545e3
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Jul 29 20:47:10 2012 +0300

    fix various fPIC issues

commit 89cb20587977b8702a1f96591852feac394ecc4b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Jul 15 20:22:50 2012 +0300

    spu: rework irqs, again..
    
    - no early irqs, breaks games that setup irq on last block and then DMA
    - don't cut last sample on bIRQReturn, fixes stereo under-decode

commit efce366c5b6596b9542c4fe8c69695191dab5d5a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jul 9 02:00:18 2012 +0300

    spu: handle cd decode buffers too

commit b72f17a1320e9038c8416343ce270b0f676c757c
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jul 9 00:28:04 2012 +0300

    spu: add decode buffer and reverb read handling

commit e4f075af4e4ba79332c72809d3bb4ba6e2895253
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jul 7 21:45:00 2012 +0300

    spu: rework irq handling and loop reloading
    
    irq might be too early, but this seems to be better than was before.
    Special thanks to Ryphecha for some ideas again.

commit f2aec10aa8e3befe7e89070e8559d070835cc23e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jul 6 20:40:38 2012 +0300

    spu: get rid of iSPUIRQWait
    
    there should be no reason to ever turn it off

commit fb552464f06137102fd9ca69a05492265bbdcea7
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jul 6 20:25:51 2012 +0300

    spu: get rid of iSpuAsyncWait hack
    
    ..and replace with some different dma detection hack

commit f8edb5bc279ba09aa7bc762da51236fd368827b0
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jul 4 19:53:30 2012 +0300

    spu: remove some hacks, to be reworked
    
    get rid of always-iSpuAsyncWait and iSecureStart hacks.
    Also SoundGetBytesBuffered is lying, make that lie more obvious
    and remove some confusing pointless constants.

commit 6d75977b13fede33db381324c610561c6e723a8b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jul 4 18:25:11 2012 +0300

    spu: remove all threading code
    
    it is broken for a while now, and I have no intention to fix it.

commit 8048a6d763a5d74c6887ea511ba43e3124cd1bff
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jun 25 01:22:13 2012 +0300

    spu: keep generating irqs while they are pending
    
    pending should be implemented as such, but there is no psemu API for it..

commit 3a721c1f5582bac7475788bb0018207a3deb2bca
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Oct 29 01:09:36 2011 +0300

    spu: add asm mixing code for ARMv5 too

commit 8cd23d6dc354bd82c9ba3fa6362c8e9fb0b50f7d
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Oct 10 01:35:15 2011 +0300

    spu: fix gaussian interpolation

commit 554a2220f2413cc29d7912ad3f7d91c9e64284cf
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Oct 9 20:50:16 2011 +0300

    psxcounters: change spu update ~2ms
    
    old value ~1.4 was weird anyway

commit 76d1d09c1224af8d663ce63dc5b32425bd62cb29
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Oct 6 00:22:46 2011 +0300

    spu: make "simple" interpolation even more simple
    
    that division is not worth the quality increase it might give,
    also sinc now can be 0 after resets, causing div0 fault.

commit 983a7cfdccceaa612267a1b045110b4f831d1495
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Oct 5 02:11:18 2011 +0300

    spu/cdda: sync cdda to spu playback
    
    this will probably make cdrom/vsync timing desync situation
    worse (those were never in sync anyway) but fix some cdda
    related sound stuttering.
    Those magic values are used trying to preserve other SPU plugin
    compatibility.

commit a80ae4a0353fce94df700ec84222d3c56c3d813a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Sep 23 19:29:03 2011 +0300

    support armv5 build

commit be1cb678c445cb8d9a4d707c698f0dc600a490ea
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Sep 22 22:38:12 2011 +0300

    spu: handle loop write vs loop flag race (bIgnoreLoop alternative)
    
    it feels this should be better, but shouldn't write much here without
    testing really (Heart of Darkness).

commit 7e44d49d2a65d6e45d6f99f72a8cccdb02654aae
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Sep 22 20:26:34 2011 +0300

    spu: clean up unneeded code

commit 77d6fd631e4c4efc26333ae43888855b9961493e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Sep 22 19:20:35 2011 +0300

    spu: use last written loop value when starting
    
    Now that we play silent channels, at least Alien Ressurection wants
    this. I should really bring out my PSX and test this properly..

commit 174c454a98a71475b72958c9f76293af7d6fb502
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Sep 21 04:03:48 2011 +0300

    spu: get rid of pCurr -1, pass right chans to silent handler
    
    pCurr -1 would work weird with savestates and could screw things up in
    general..

commit 587fa7de9b496a0b7fa1d52d393733886602ca83
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Tue Sep 20 02:29:28 2011 +0300

    spu: fix a few mixing issues

commit a3ed191507a8162f676f92b44dd61eb5e14d2194
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Sep 16 16:55:31 2011 +0300

    spu: don't wait for irq on silent channels
    
    no point to do so, breaks other chans if silent ones
    interrupt often (Discworld)

commit b17618c006c1a79ea0aa38c5e452a384f7f2bc95
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Aug 1 14:07:33 2011 +0300

    spu: split out mixing, use NEON for that on ARM

commit b1094d0ef5dce90b6c344e7616ca0acd10050c7e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Jul 31 00:17:17 2011 +0300

    spu: merge DrHell/shalma noise code
    
    fraction part dropped since it looked suspicious.

commit 9e7a735287d03149ccf7d0db6c0193cc565c3fff
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jul 30 23:30:26 2011 +0300

    spu: change volume control, default it to 3/4 instead of 1/2
    
    because some complain default volume is too low

commit 381ea10346ab85b2f2d46f33d28461211e239c0a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jul 30 02:42:28 2011 +0300

    spu: some misc refactoring/cleanup

commit 1775933abd26d2e282c4e0b4093778d204a0038d
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jul 30 02:38:52 2011 +0300

    spu: some major reverb refactoring
    
    some ideas lifted from SPU2-X (Pcsx2)

commit 07a6dd2ce2c0c8ea2de11c30c134c877e7c7b0fb
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jul 29 02:24:05 2011 +0300

    spu: parametrize sample generation, interpolation

commit 1ab7621a76d1ef82fde77322c12d4076889bed01
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jul 29 00:31:56 2011 +0300

    spu: do adsr on whole buffer
    
    this eliminates need to check adsr state for each sample,
    we can do multiple at once.
    Also use ADSR tables from DrHell/shalma.

commit 90f1d26c2e771993c1a563f4c0ea81432bd9fc5e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jul 29 00:25:41 2011 +0300

    frontend: add chan display option, for fun mostly

commit 78c60846942cad7d41d6d23a7c3d2b905ee4b822
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jul 27 20:10:49 2011 +0300

    spu: handle non-playing channel irqs

commit 3fc2a4c27be8c8cd5f8cf53a0d56c263275b8501
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jul 27 20:06:46 2011 +0300

    spu: simplify irq and flag handling
    
    shalma's recent work taken into account when working on this.
    NOT verified on read hardware.

commit e11ae5c58194de196c57ba0a2107d6eec5c0394a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jul 27 01:33:49 2011 +0300

    spu: some refactoring for upcoming optimization attempt

commit a02dc3f33f8ac5ac22106e3417cac0d736628cd4
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Jul 27 01:31:21 2011 +0300

    spu: remove adsr stop prevention hack
    
    it's is not enough for Tron Bonne, so is not useful

commit 5238f6f8fcf43fb8506467b358ee976a8c5fdf6b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Fri Jul 1 02:15:08 2011 +0300

    dfsound: merge shalma's suggestions
    
    http://www.gp32x.com/board/index.php?/topic/57973-pcsx-rearmed/page__view__findpost__p__952591

commit b8e744ce04eaf294d0c06aca6b20b410122bf4ed
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Mar 7 19:56:42 2011 +0200

    spu: fix valgrind errors

commit 97ea407715c0eafc8d73ffc5477f6d7e6ad8f20b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Tue Mar 1 00:48:20 2011 +0200

    spu: drop unused mono code
    
    also merge the mix buffers - the idea was to make use of neon there but
    I've decided againt it, it wouldn't improve things much according to
    profiling.

commit 9cf0ddbcdf4a2d72348df35695d09b36d23a2d0a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Feb 28 02:13:23 2011 +0200

    spu: refactor adsr (simplify code)

commit 6d866bb7894b252fc430a24a97769511d5ead1ca
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Feb 28 00:19:48 2011 +0200

    spu: compact chan structure a bit

commit 17ed0d696a7a3649cc41dcf182e162c85002b33a
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Feb 26 23:15:43 2011 +0200

    gte_neon: implement MVMVA, some fixes

commit cdb31c9557f464b452ce6dc78b58b7e73cd423d8
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Tue Feb 8 01:32:53 2011 +0200

    frontend: enable SPUIRQWait by default

commit 8680e8229414b74335e2c1d05cc678cf8bab685b
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Tue Feb 8 01:27:36 2011 +0200

    spu: handle channels better in 'IRQ wait' case

commit b00afb7734d75d4e625db0263548fa15b9aa0af7
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Wed Feb 9 14:34:15 2011 +0200

    dfsound: backport fixes from pcsxr
    
    shalma:
    - Align start addresses to 16-bytes
    - (Hopkat) registers.c - External loop address align to 16 bytes
    - SoundOff prevents new channel from starting
    - fixes Jungle Book (buzzing loop hangs)

commit 9098b863732585e49d22dba266c4f342ed1ecaa6
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Feb 3 23:56:14 2011 +0200

    spu: fix xa masking, was causing noise

commit 08cfd5e5b3027af766dab3916747c0fc9ae47d7d
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 29 19:47:46 2011 +0200

    spu: don't stop a channel by adsr if we have potential irq
    
    otherwise we might miss irqs

commit ee849648a1bd5d12bd48c98f3b51bb96d7984d43
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Jan 29 19:45:37 2011 +0200

    spu: cleanup build a bit

commit d7296e10805869f9772afc77d31ab4f7715e6e82
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Mon Jan 24 00:37:03 2011 +0200

    spu: don't read config, we manage it in frontend

commit cfe71c00d61819b76744f7163316d98cc389c91e
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sat Dec 25 00:27:55 2010 +0200

    spu: kill voldiv on ARM too

commit 367b1b93e323bd0a8e1b5ad59cc07a4e7f296fd6
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Sun Dec 12 18:44:00 2010 +0200

    spu: disable SPUIRQWait, it only seems to cause problems

commit e906c010e1bea71ed4df425be97ce45dc777818c
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Thu Nov 18 15:08:15 2010 +0200

    build plugins in

commit aa1fc44b4532dc68be2482730a275f8a3ba8a039
Author: notaz <notasas <AT> gmail (DOT) com>
Date:   Tue Nov 16 18:39:07 2010 +0200

    remove stuff we won't need
    
    this makes the source tree more pleasant to work with.

commit ef79bbde537d6b9c745a7d86cb9df1d04c35590d
Author: PCSX* teams <PCSX>
Date:   Tue Nov 16 14:15:22 2010 +0200

    pcsxr-1.9.92

// END GIT COMMIT LOG
