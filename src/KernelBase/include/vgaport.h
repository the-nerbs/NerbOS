//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Defines the VGA IO ports.
//-------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------
//! \brief  Defines the various VGA IO port values.
//!
//! \details
//! |   Read    |   Write   |   Register                                    |
//! |:---------:|:---------:|-----------------------------------------------|
//! |   0x3CC   |   0x3C2   |   Miscellaneous Output Register               |
//! |   0x3C2   |     -     |   Input Status 0 Register                     |
//! |   0x3?A   |     -     |   Input Status 1 Register                     |
//! |   0x3CA   |   0x3?A   |   Feature Control Register                    |
//! |   0x3C4   |   0x3C4   |   Sequencer Index Register                    |
//! |   0x3C5   |   0x3C5   |   Sequencer Data Register                     |
//! |   0x3?4   |   0x3?4   |   CRT Controller (CRTC) Index Register        |
//! |   0x3?5   |   0x3?5   |   CRT Controller (CRTC) Data Register         |
//! |   0x3CE   |   0x3CE   |   Graphics Controller Index Register          |
//! |   0x3CF   |   0x3CF   |   Graphics Controller Data Register           |
//! |   0x3C0   |   0x3C0   |   Attribute Controller Index Register         |
//! |   0x3C1   |   0x3C0   |   Attribute Controller Data Register          |
//! |   0x3C8   |   0x3C8   |   Video DAC Index (CLUT Writes)               |
//! |     -     |   0x3C7   |   Video DAC Index (CLUT Reads)                |
//! |   0x3C9   |   0x3C9   |   Video DAC Data Register                     |
//! |   0x3C6   |   0x3C6   |   Video DAC Pel Mask Register                 |
//! 
//! Question marks indicate that the addresses change based on bit 0 of the Miscellaneous
//! Output Register. If clear, these registers are at 0x3Bx, and 0x3Dx when set. We keep this
//! bit set.
//!
//! \note
//! 0x3C0 alternates between index and data with every byte written. Reading from 0x3?A will
//! reset this back to expecting an index byte.
//-------------------------------------------------------------------------------------------------
enum VgaIoPort
{
    VGAIO_MiscOutputIn = 0x3CC,
    VGAIO_MiscOutputOut = 0x3C2,

    VGAIO_InputStatus0In = 0x3C2,

    VGAIO_InputStatus1In = 0x3DA,

    VGAIO_FeatureControlIn = 0x3CA,
    VGAIO_FeatureControlOut = 0x3DA,

    VGAIO_AttrIndexIn = 0x3C0,
    VGAIO_AttrIndexOut = 0x3C0,

    VGAIO_AttrDataIn = 0x3C1,
    VGAIO_AttrDataOut = 0x3C0,

    VGAIO_CRTControlIndex = 0x3D4,
    VGAIO_CRTControlData = 0x3D5,
};
