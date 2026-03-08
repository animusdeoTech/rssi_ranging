# RSSI-Based Ranging and Localization with LoRa

Master's thesis project — KU Leuven, 2021.

A prototype system for estimating the position of a mobile node using RSSI (Received Signal Strength Indicator) measurements from a network of fixed LoRa anchor nodes. The system collects RF signal data across varying spreading factors, fits a log-distance path loss model, and applies trilateration to estimate 2D position.

---

## Motivation

RSSI-based ranging is a low-cost alternative to time-of-flight or angle-of-arrival positioning. LoRa's long range, low power consumption, and configurable spreading factor make it an interesting candidate for outdoor localization. This project investigates how well RSSI-to-distance mapping works in practice, and how accurately a tag's position can be recovered from three anchor RSSI measurements.

---

## System Architecture

The system consists of three components: a mobile tag, three fixed anchor nodes, and an offline data analysis pipeline.

```
         [ Tag ]
           |
     LoRa broadcast
    /       |       \
[Anchor A] [Anchor B] [Anchor C]
    |           |           |
  Serial      Serial      Serial
    |           |           |
  [RPi A]    [RPi B]    [RPi C]
    |           |           |
  SQLite      SQLite      SQLite
           \   |   /
         [data_analysis/]
```

### Tag

An Arduino-based node with an RFM95 LoRa radio and a GPS module. It continuously transmits packets while sweeping spreading factors 7 through 12. Before each SF sweep it broadcasts a configuration packet (frequency, current SF, GPS coordinates) so anchors know the transmission parameters. It then sends 100 numbered data packets per SF setting.

### Anchor Nodes

Three fixed nodes, each consisting of a Raspberry Pi connected to an Arduino with an RFM95 radio. The Arduino receives LoRa packets from the tag, records RSSI and SNR for each packet, and relays the measurements over serial to the RPi. Each anchor also has a GPS module to log its own position.

The three anchor sketches (`anchor_1`, `anchor_2`, `anchor_3`) are functionally identical but compiled and flashed separately. They listen for a configuration packet from the tag, then collect measurements for approximately 5 seconds per SF setting.

**LoRa parameters:**
- Frequency: 863.1 MHz (ISM band)
- Spreading factors: SF7–SF12
- Coding rate: 4/4
- TX power: 20 dBm

### Data Pipeline

Each RPi runs `serial_read.py`, which reads binary-encoded structs from the Arduino over serial and inserts them into a local SQLite database. Each row in the `exp_final` table stores: RSSI, SNR, tag GPS coordinates, anchor GPS coordinates, frequency, and spreading factor.

Post-collection analysis is done offline in Jupyter notebooks.

---

## Repo Structure

```
rssi_ranging/
├── anchor/
│   ├── anchor.ino              # Base anchor sketch
│   ├── anchor1/anchor_1/       # Anchor node 1 sketch (with SNR + keepalive)
│   ├── anchor2/anchor_2/       # Anchor node 2 sketch
│   └── anchor3/anchor_3/       # Anchor node 3 sketch
├── tag/
│   └── tag.ino                 # Mobile tag sketch (SF sweep + GPS)
├── rpi_scripts/
│   └── serial_read.py          # Serial → SQLite data collector
├── data_analysis/
│   ├── trilateration.ipynb     # Path loss modeling + trilateration
│   ├── data_analysis.ipynb     # Statistical analysis of RSSI variability
│   ├── main.py                 # 3D RSSI std dev analysis across SF/frequency
│   └── splines.py              # Natural cubic spline regression utility
├── RadioHead/                  # RadioHead library (LoRa driver)
├── TinyGPS/                    # TinyGPS library (GPS parsing)
├── A.db / B.db / C.db          # Per-anchor SQLite datasets
├── rssi.db / rssi_1.db / ...   # Additional experiment datasets
└── README.md
```

---

## Tech Stack

| Layer | Technology |
|---|---|
| Firmware | C++ / Arduino (RH_RF95 via RadioHead) |
| GPS parsing | TinyGPS library |
| Data collection | Python 3, PySerial, sqlite3 |
| Analysis | Jupyter, pandas, NumPy, scikit-learn |
| Visualization | Matplotlib |
| Distance calculation | Haversine formula |
| Storage | SQLite |

---

## Data Analysis

### Path Loss Model

Distance is estimated from RSSI using a log-distance path loss model:

```
RSSI = C - 10 * n * log10(d)
```

Where `n` is the path loss exponent and `C` is a reference RSSI at 1 meter. Both parameters are fitted per-experiment using linear regression on `(log10(d), RSSI)` pairs derived from GPS ground truth.

A height offset of 12.36 m (antenna height difference between tag and anchors) is included in all ground-truth distance calculations via the Haversine formula extended to 3D.

### Trilateration

Given estimated ranges `r1, r2, r3` from three anchors at known positions, the tag position `(x, y)` is recovered by solving the linearized system:

```
A·x + B·y = C
D·x + E·y = F
```

This is a closed-form solution derived from the three circle intersection equations. The notebooks test this on real data and compare estimated positions to GPS ground truth.

### Spreading Factor Analysis

`data_analysis.ipynb` and `main.py` analyze how RSSI standard deviation varies across spreading factors (SF7–SF12) and frequency. Higher spreading factors improve link budget but do not necessarily reduce RSSI variance.

---

## Limitations and Results

**Multipath fading** is the dominant source of error. In an outdoor environment, reflected and diffracted signal copies interfere constructively and destructively, causing the received power to fluctuate significantly even at fixed distance. This results in high RSSI variance (often ±5–10 dBm at a given distance), which translates directly to large ranging errors and ultimately poor position estimates.

Key observations:

- RSSI is a noisy proxy for distance. The log-distance model fits the mean trend but individual measurements scatter widely around it.
- Averaging multiple packets per position reduces variance somewhat but does not eliminate it.
- Trilateration amplifies ranging errors: small errors in each individual distance estimate compound into larger position errors, particularly when the anchor geometry is suboptimal (poor dilution of precision).
- Higher spreading factors (SF11, SF12) provide better SNR but do not systematically improve ranging accuracy compared to lower SFs in this deployment.

The prototype demonstrates that LoRa RSSI can give a rough position estimate (order of tens of meters) but is unsuitable for applications requiring accuracy below ~10 m without additional filtering, calibration, or sensor fusion.

---

## Dependencies

**Arduino libraries** (included in repo):
- [RadioHead](http://www.airspayce.com/mikem/arduino/RadioHead/) — RF95 LoRa driver
- [TinyGPS](http://arduiniana.org/libraries/tinygps/) — NMEA GPS parser

**Python packages:**
```
pyserial
pandas
numpy
matplotlib
scikit-learn
haversine
jupyter
```

---

## Hardware

- Arduino (Uno or compatible) with RFM95 LoRa module (SPI)
- Raspberry Pi (any model with USB serial)
- GPS module on SoftwareSerial (RX: pin 4, TX: pin 3)
- Three anchor deployments at known, GPS-surveyed locations
