# Functional Near-Infrared Spectroscopy (fNIRS) Acquisition System
## Overview
- This repository contains the core software and documentation for a custom-built functional near-infrared spectroscopy (fNIRS) system developed in-house. It interfaces with the National Instruments USB-6351 DAQ and utilizes a custom optode array comprising 16 detectors and a configurable number of emitters (sources) using HL6738MG (690 nm) and HL8338MG (830 nm) diodes.

- The system is designed to support flexible optical topographies, real-time acquisition, and raw signal logging, with post-processing pipelines available for hemoglobin concentration estimation.

### Features
- 🎛 Hardware Integration: Interfaces with the NI USB-6351 multifunction DAQ.

- 🔦 Flexible Emitter Configuration: Supports arbitrary emitter timing and modulation patterns.

- 📡 16 Photodetector Support: Synchronizes readings from 16 analog photodiode channels.

- 🧠 fNIRS Signal Acquisition: Captures light intensity data suitable for converting to HbO/HbR using modified Beer-Lambert Law.

- 📈 Real-Time Visualization (optional): Real-time plots of raw signal for system diagnostics.

- 💾 Data Logging: Outputs timestamped raw intensity values per channel.

--- 

### Hardware Setup
DAQ: NI USB-6351 

Photodetectors: 16 analog input channels (AI0–AI15)

Emitters: HL6738MG (690 nm) and HL8338MG (830 nm) diodes, up to 24 emitters supported


## License
This is a proprietary research system developed in-house. Not for commercial use or redistribution without written permission.