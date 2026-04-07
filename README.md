These are the source files for building the MMDVM Host, the program that
interfaces to the MMDVM or DVMega on the one side, and a suitable network (via a
gateway) on the other. It supports D-Star, DMR, P25 Phase 1, NXDN, System Fusion,
POCSAG, and FM on the MMDVM, and D-Star, DMR, and System Fusion on the DVMega.

On the D-Star side the MMDVM Host interfaces with the D-Star Gateway, on DMR it
connects to the DMR Gateway to allow for connection to multiple DMR networks.
On System Fusion it connects to the DG-Id or YSF Gateway to allow
access to the FCS and YSF networks. On P25 it connects to the P25 Gateway. On
NXDN it connects to the NXDN Gateway which provides access to the NXDN and
NXCore talk groups. It uses the DAPNET Gateway to access DAPNET to receive
paging messages for transmission using POCSAG.

It builds on 32-bit and 64-bit Linux as well as on Windows using Visual Studio
2022 on x86 and x64.

This software is licenced under the GPL v2 and is primarily intended for amateur and
educational use.
