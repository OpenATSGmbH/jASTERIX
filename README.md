# jASTERIX

jASTERIX is a C++ Library for EUROCONTROL's [ASTERIX](https://www.eurocontrol.int/services/asterix) to [JSON](https://www.json.org/) conversion. It is part of the [OpenATS COMPASS project](https://github.com/hpuhr/COMPASS/), but released as a separate library to allow easy usage in other projects.

The library allows decoding of binary ASTERIX data into JSON. The ASTERIX definitions are configuration only, so additional or customized categories or editions can be added without having to recompile. Also, using the Intel TBB library allows for reasonable performance using multi-threading.

For a detailed list of features, as well as the offered commercial services, please refer to [OpenATS jASTERIX](https://www.openats.at/projects/jasterix/).

## Supported Editions

| CAT |         Editions | REFs |       SPFs |
|-----|------------------|------|------------|
| 001 |              1.1 |      |            |
| 002 |              1.0 |      |            |
| 004 |              1.4 |      |            |
| 010 |             0.31 |      |            |
| 019 |         1.2, 1.3 |      |            |
| 020 |         1.5, 1.8 |  1.3 |            |
| 021 |   0.26, 2.1, 2.4 |      |            |
| 023 |              1.2 |      |            |
| 030 |              7.0 |      |            |
| 034 |             1.26 |      |            |
| 048 |       1.15, 1.23 |  1.9 |            |
| 062 | 1.12, 1.16, 1.18, 1.21 |  1.2 | ARTAS TRIs |
| 063 |          1.0,1.1 |      |            |
| 065 |         1.2, 1.3 |      |            |
| 247 |              1.2 |      |            |
| 252 |              7.0 |      |            |



## Contents
- Folder "analyze": Contains Python scripts for data analysis
- Folder "appimage": Contains files required for AppImage building
- Folder "cmake_modules": Contains cmake find scripts
- Folder "definitions": Contains ASTERIX configuration definitions
- Folder "include": Main headers
- Folder "lib": User header-only libraries
- Folder "src": Contains source code
- CMakeLists.txt: CMake config file
- LICENSE: GPL license
- README.md: This file

## Requirements / Used Libraries

The following libraries are mandatory:
- Boost
- Intel Thread Building Blocks
- LibArchive
- libpcap

The following libaries are optional (can be deactivated in CMakeList.txt):
- Log4Cpp
- OpenSSL

Also, the Nlohmann::JSON and the Catch2 libraries are used.

## Client Installation without Building

Since v0.0.3 an client AppImage is supplied, which can be executed (without setup effort) under all recent Linux distributions (since Ubuntu 14.04).

Download the AppImage and the jASTERIX definitions from the [Releases](https://github.com/hpuhr/jASTERIX/releases) page, and extract the definitions into a local folder, e.g. 'definitions'. Execute the following command to add the executable flag to the AppImage:

```
chmod +x jASTERIX_client_v0.0.3-x86_64.AppImage
```

After this, the jASTERIX client can be run from the console (see Usage section).

## Installation with Building

Download the source code from this page, then execute the following commands in the check-out folder:
```
mkdir build
cd build
cmake ..
make
```

To install the built libary in the system, execute the following command in the build folder:
```
sudo make install
```

## Usage

### Help

After building, the following jASTERIX client can be used (from the check-out folder):
```
./jASTERIX_client-x86_64.AppImage --help
INFO    : Allowed options:
  --help                   produce help message
  --filename arg           input file name
  --definition_path arg    path to jASTERIX definition files
  --framing arg            input framine format, as specified in the framing 
                           definitions. raw/netto is default
  --frame_limit arg        number of frames to process with framing, default 
                           -1, use -1 to disable.
  --frame_chunk_size arg   number of frames to process in one chunk, default 
                           1000, use -1 to disable.
  --data_block_limit arg   number of data blocks to process without framing, 
                           default 10000, use -1 to disable.
  --data_write_size arg    number of frame chunks to write in one file write, 
                           default 1, use -1 to disable.
  --debug                  print debug output (only for small files)
  --debug_include_framing  print debug output including framing, debug still 
                           has to be set, disable per default
  --single_thread          process data in single thread
  --only_cats arg          restricts categories to be decoded, e.g. 20,21.
  --editions arg           set non-default editions per category, e.g. 
                           21:0.26,48:1.15.
  --pcap                   input file is a PCAP capture (libpcap); ASTERIX 
                           payload is extracted and decoded as raw/netto (no 
                           framing).
  --log_perf               enable performance log after processing
  --add_artas_md5          add ARTAS MD5 hashes
  --check_artas_md5 arg    add and check ARTAS MD5 hashes (with record data), 
                           stating which categories to check, e.g. 1,20,21,48
  --flat                   output in flat/columnar format (cat -> leaf_name -> 
                           array)
  --add_record_data        add original record data in hex
  --print                  print JSON output
  --print_indent arg       intendation of json print, use -1 to disable.
  --write_type arg         optional write type, e.g. text,zip. needs 
                           write_filename.
  --write_filename arg     optional write filename, e.g. test.zip.
```
### Decoding Test & Performance

To test decode and measure performance, use:
```
./jASTERIX_client-x86_64.AppImage --definition_path definitions/ --filename example.ff --framing ioss --log_perf
INFO    : jASTERIX client: decoded 674150 frames, 1073956 records in 0h 0m 8s 936ms: 75442 fr/s, 120183 rec/s
```

Performance depends on the used hardware, frame_chunk_size and ASTERIX data content. On the author's workstation (Intel i7 4790k) the performance varies between 50k-150k rec/s.

### Decode & Print

To decode and print an ASTERIX file as JSON text, use:
```
./jASTERIX_client-x86_64.AppImage --definition_path definitions/ --filename src/test/cat020ed1.5.bin --print
{
    "data_blocks": [
        {
            "category": 20,
            "content": {
                "index": 3,
                "length": 98,
                "records": [
                    {
                        "010": {
                            "SAC": 0,
                            "SIC": 2
                        },
                        "020": {
                            "CHN": 0,
                            "CRT": 0,
                            "DME": 0,
                            "FX": 1,
                            "FX2": 0,
                            "GBS": 0,
                            "HF": 0,
                            "MS": 1,
                            "OT": 0,
                            "RAB": 0,
                            "SIM": 0,
                            "SPI": 0,
                            "SSR": 0,
                            "TST": 0,
                            "UAT": 0,
                            "VDL4": 0
                        },
                        "041": {
                            "Latitude": 47.88232132925,
                            "Longitude": 16.32056296698
                        },
                        "042": {
                            "X": 173529.5,
                            "Y": 45109.0
                        },
                        "070": {
                            "G": 0,
                            "L": 1,
                            "Mode-3/A code": 7000,
                            "V": 0
                        },
                        "090": {
                            "Flight Level": 11.25,
                            "G": 0,
                            "V": 0
                        },
                        "140": {
                            "Time of Day": 33502.7109375
                        },
                        "161": {
                            "Track Number": 3528
                        },
                        "170": {
                            "CDM": 3,
                            "CNF": 0,
                            "CST": 0,
                            "FX": 0,
                            "MAH": 0,
                            "STH": 0,
                            "TRE": 0
                        },
                        "202": {
                            "Vx": -13.75,
                            "Vy": -9.25
                        },
                        "210": {
                            "Ax": 0.0,
                            "Ay": 0.0
                        },
                        "220": {
                            "Target Address": 148527
                        },
                        "230": {
                            "AIC": 0,
                            "ARC": 1,
                            "B1A": 0,
                            "B1B": 0,
                            "COM": 1,
                            "MSSC": 0,
                            "STAT": 0
                        },
                        "250": {
                            "Mode S MB Data": [
                                {
                                    "BDS1": 1,
                                    "BDS2": 0,
                                    "MB Data": "10000000a00000"
                                },
                                {
                                    "BDS1": 1,
                                    "BDS2": 7,
                                    "MB Data": "00000000000000"
                                }
                            ],
                            "REP": 2
                        },
                        "400": {
                            "Contributing Receivers": [
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 16
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 32
                                },
                                {
                                    "RUx": 0
                                },
                                {
                                    "RUx": 34
                                }
                            ],
                            "REP": 16
                        },
                        "FSPEC": [
                            true,
                            true,
                            true,
                            true,
                            true,
                            true,
                            true,
                            true,
                            true,
                            true,
                            true,
                            false,
                            true,
                            false,
                            false,
                            true,
                            false,
                            true,
                            false,
                            false,
                            false,
                            true,
                            true,
                            true,
                            true,
                            false,
                            false,
                            false,
                            false,
                            true,
                            false,
                            false
                        ],
                        "REF": {
                            "PA": {
                                "DOP": {
                                    "DOP-x": 4.5,
                                    "DOP-xy": -3.75,
                                    "DOP-y": 3.75
                                },
                                "SDC": {
                                    "COV-XY (Covariance Component)": -30.5,
                                    "SDC (X-Component)": 34.25,
                                    "SDC (Y-Component)": 31.0
                                },
                                "SDW": {
                                    "COV-WGS (Lat/Long Covariance Component)": -0.00033795783,
                                    "SDW (Latitude Component)": 0.00028431372999999997,
                                    "SDW (Longitude Component)": 0.00044524603
                                },
                                "available": [
                                    true,
                                    true,
                                    false,
                                    true,
                                    false,
                                    false,
                                    false,
                                    false
                                ]
                            },
                            "REF_FSPEC": [
                                true,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false
                            ]
                        },
                        "index": 3,
                        "length": 98
                    }
                ]
            },
            "length": 101
        }
    ]
}
```

Decoded JSON can also be written to a text or ZIP file using the appropriate arguments.

### Flat / Columnar Output

By default the JSON output is structured (nested per record, mirroring the ASTERIX data block / record hierarchy as shown above). Using the `--flat` argument, the output is instead given in a flat, columnar layout: keyed by category number, each leaf field becomes a top-level array with one entry per record (indexed by record order). All arrays of a category have the same length; fields not present in a record (not selected by its FSPEC) are `null` at that position.

```
./jASTERIX_client-x86_64.AppImage --definition_path definitions/ --filename src/test/cat048ed1.15.bin --flat --print
{
    "48": {
        "010.SAC": [0, 0, 5],
        "010.SIC": [1, 1, 2],
        "040.RHO": [73.92, null, 55.10],
        "040.THETA": [89.67, null, 120.33],
        "070.Mode-3/A reply": [470, 471, null],
        "090.Flight Level": [370.0, 365.0, null],
        "140.Time-of-Day": [33499.84, 33500.12, 33500.50],
        "220.AIRCRAFT ADDRESS": [11226301, 11226301, null]
    }
}
```

This format is convenient for direct import into columnar/data-frame tooling (e.g. pandas, Apache Arrow). It is available both via the CLI `--flat` flag and the library API (the `do_flat` parameter of `decodeFile()` / `decodeData()` / `decodePCAPFile()`).

### PCAP Captures

ASTERIX data captured in a PCAP file (e.g. recorded with `tcpdump`/Wireshark) can be decoded directly using the `--pcap` argument. The ASTERIX payload is extracted from the network streams and decoded as raw/netto (no framing). Per-packet capture timestamps are tracked during decoding.

```
./jASTERIX_client-x86_64.AppImage --definition_path definitions/ --filename capture.pcap --pcap --print
```

`--pcap` combines with the other arguments, e.g. `--flat`, `--only_cats`, `--write_type`/`--write_filename`, and `--analyze`/`--analyze_csv`.

In the library API the corresponding entry points are:
- `decodePCAPFile(filename, callback, do_flat)` — decode a PCAP capture, delivering JSON chunks to the callback
- `analyzePCAPFile(filename, record_limit)` — return a JSON summary of the data sources/contents in a capture
- `analyzePCAPFileCSV(filename, record_limit)` — same analysis, returned as CSV text

### Analysis

In the 'analyze' folder, example Python scripts are given which can be used to analyse decoded JSON in a simple but powerful manner. Using the decoder and the print function, JSON data is piped into a Python script, where it can be analysed. A user could simply write the own scripts for deep data analysis.

#### Data Item Analysis

The Python script 'analyze/data_items.py' allows for the following options:

```
python3 analyze/data_items.py --help
usage: data_items.py [-h] --framing FRAMING [--cats CATS]

ASTERIX data item analysis

optional arguments:
  -h, --help         show this help message and exit
  --framing FRAMING  Framing True or False
  --cats CATS        ASTERIX categories to be analyzed as CSV
```

To analyze the data items in a recording, start the jASTERIX decoder and pipe the output in the the script with the appropriate arguments.

The following example analyzes the CAT048 content in the first 100000 frames:
```
./jASTERIX_Client-x86_64.AppImage --definition_path definitions/ --filename example.ff --framing ioss --print --print_indent -1 --only_cats 48 --frame_limit 100000 | python3 analyze/data_items.py --framing True
framing True
cats [48]
blocks 1 time 0.28s records 73 filtered 652 rate 2601 rec/s
...
blocks 10 time 2.24s records 813 filtered 7718 rate 3813 rec/s
num records 813
data items

'048' count 813
'048.010' count 813 (100%)
'048.010.SAC' count 813 (100%) min '0' max '1'
'048.010.SIC' count 813 (100%) min '0' max '2'
'048.020' count 813 (100%)
'048.020.FX' count 813 (100%) min '0' max '0'
'048.020.RAB' count 813 (100%) min '0' max '0'
'048.020.RDP' count 813 (100%) min '0' max '0'
'048.020.SIM' count 813 (100%) min '0' max '0'
'048.020.SPI' count 813 (100%) min '0' max '0'
'048.020.TYP' count 813 (100%) min '2' max '3'
'048.040' count 813 (100%)
'048.040.RHO' count 813 (100%) min '9.703125' max '249.6875'
'048.040.THETA' count 813 (100%) min '0.03295898436' max '359.54956038323996'
'048.042' count 60 (7%)
'048.042.X-Component' count 60 (100%) min '-159.71875' max '163.96875'
'048.042.Y-Component' count 60 (100%) min '-169.078125' max '72.125'
'048.070' count 813 (100%)
'048.070.G' count 813 (100%) min '0' max '0'
'048.070.L' count 813 (100%) min '0' max '0'
'048.070.Mode-3/A reply' count 813 (100%) min '21' max '6732'
'048.070.V' count 813 (100%) min '0' max '1'
'048.090' count 807 (99%)
'048.090.Flight Level' count 807 (100%) min '18.0' max '410.0'
'048.090.G' count 807 (100%) min '0' max '0'
'048.090.V' count 807 (100%) min '0' max '0'
'048.140' count 813 (100%)
'048.140.Time-of-Day' count 813 (100%) min '71.8046875' max '1578.484375'
'048.161' count 60 (7%)
'048.161.TRACK NUMBER' count 60 (100%) min '1802' max '1847'
'048.170' count 60 (7%)
'048.170.CDM' count 60 (100%) min '0' max '2'
'048.170.CNF' count 60 (100%) min '0' max '0'
'048.170.DOU' count 60 (100%) min '0' max '0'
'048.170.FX' count 60 (100%) min '1' max '1'
'048.170.GHO' count 60 (100%) min '0' max '0'
'048.170.MAH' count 60 (100%) min '0' max '1'
'048.170.RAD' count 60 (100%) min '0' max '2'
'048.170.SUP' count 60 (100%) min '0' max '0'
'048.170.TCC' count 60 (100%) min '1' max '1'
'048.170.TRE' count 60 (100%) min '0' max '0'
'048.200' count 60 (7%)
'048.200.CALCULATED GROUNDSPEED' count 60 (100%) min '325.41500602113604' max '567.553653310327'
'048.200.CALCULATED HEADING' count 60 (100%) min '81.48559566604' max '348.71704085692'
'048.SPF' count 60 (7%) min '400101ba00' max '44d3006e0004'
'048.index' count 813 (100%) min '34' max '3301579'
'048.length' count 813 (100%) min '13' max '37'
```

#### ADS-B Quality Indicators Analysis

The Python script 'analyze/adsb_quality.py' allows for the following options:

```
usage: adsb_quality.py [-h] --framing FRAMING

ASTERIX data item analysis

optional arguments:
  -h, --help         show this help message and exit
  --framing FRAMING  Framing True or False
```

The following target report counters are calculated:
- MOPS Version is supported by the GS: Counters of supported by the ADS-B ground station
- MOPS Version Number: Counters of MOPS versions
- Emitter Category: Counters of emitter category (aircraft type)
- Navigation Accuracy Category - Position: Counter of NACp values of DO-260A/B transponders
- MOPS Version for ECAT: Counters of MOPS versions, split up for different emitter category values
- NACp for ECAT: Counters of NACp values, split up for different emitter category values

To analyze the data items in a recording, start the jASTERIX decoder and pipe the output in the the script with the appropriate arguments.

The following example analyzes the CAT021 content in the first 100000 frames:
```
./jASTERIX_Client-x86_64.AppImage --definition_path definitions/ --filename example.ff --framing ioss --print --print_indent -1 --frame_limit 100000 | python3 analyze/adsb_quality.py --framing True
blocks 1 time 0.91s records 6461 rate 7104 rec/s
...
blocks 10 time 8.13s records 65751 rate 8087 rec/s
num ads-b records 65751

MOPS Version is supported by the GS:
	none: 0 (0.00%)
	MOPS Version is supported by the GS: 65751 (100.00%)
--------------------------------------------------------------------------------

MOPS Version Number:
	none: 0 (0.00%)
	ED102/DO-260: 21515 (32.72%)
	DO-260A: 1481 (2.25%)
	ED102A/DO-260B: 42755 (65.03%)
--------------------------------------------------------------------------------

Emitter Category:
	none: 23576 (35.86%)
	light aircraft <= 15500 lbs: 1624 (3.85%)
	75000 lbs < medium a/c < 300000 lbs: 31672 (75.10%)
	300000 lbs <= heavy aircraft: 8561 (20.30%)
	rotocraft: 318 (0.75%)
--------------------------------------------------------------------------------

Navigation Accuracy Category - Position:
	none: 21515 (32.72%)
	EPU < 10 m, VEPU < 15 m: 13395 (30.28%)
	EPU < 30 m, VEPU < 45 m: 14951 (33.80%)
	EPU < 0.05 NM (93 m): 14688 (33.20%)
	EPU < 0.1 NM (185 m): 682 (1.54%)
	EPU < 1.0 NM (1852 m): 1 (0.00%)
	EPU < 4 NM (7408 m): 1 (0.00%)
	EPU > 10 NM or Unknown: 518 (1.17%)
--------------------------------------------------------------------------------

MOPS Version for ECAT:

MOPS Version for ECAT: '300000 lbs <= heavy aircraft' records 8561 (13.02%):
	none: 0 (0.00%)
	DO-260A: 129 (1.51%)
	ED102A/DO-260B: 8432 (98.49%)

MOPS Version for ECAT: '75000 lbs < medium a/c < 300000 lbs' records 31672 (48.17%):
	none: 0 (0.00%)
	ED102/DO-260: 79 (0.25%)
	DO-260A: 1281 (4.04%)
	ED102A/DO-260B: 30312 (95.71%)

MOPS Version for ECAT: 'None' records 23576 (35.86%):
	none: 0 (0.00%)
	ED102/DO-260: 21433 (90.91%)
	DO-260A: 66 (0.28%)
	ED102A/DO-260B: 2077 (8.81%)

MOPS Version for ECAT: 'light aircraft <= 15500 lbs' records 1624 (2.47%):
	none: 0 (0.00%)
	ED102/DO-260: 3 (0.18%)
	DO-260A: 5 (0.31%)
	ED102A/DO-260B: 1616 (99.51%)

MOPS Version for ECAT: 'rotocraft' records 318 (0.48%):
	none: 0 (0.00%)
	ED102A/DO-260B: 318 (100.00%)
--------------------------------------------------------------------------------

NACp for ECAT:

NACp for ECAT: '300000 lbs <= heavy aircraft' records 8561 (13.02%):
	none: 0 (0.00%)
	EPU < 10 m, VEPU < 15 m: 2249 (26.27%)
	EPU < 30 m, VEPU < 45 m: 4988 (58.26%)
	EPU < 0.05 NM (93 m): 1274 (14.88%)
	EPU < 0.1 NM (185 m): 15 (0.18%)
	EPU > 10 NM or Unknown: 35 (0.41%)

NACp for ECAT: '75000 lbs < medium a/c < 300000 lbs' records 31672 (48.17%):
	none: 79 (0.25%)
	EPU < 10 m, VEPU < 15 m: 8799 (27.85%)
	EPU < 30 m, VEPU < 45 m: 9115 (28.85%)
	EPU < 0.05 NM (93 m): 12860 (40.71%)
	EPU < 0.1 NM (185 m): 598 (1.89%)
	EPU < 1.0 NM (1852 m): 1 (0.00%)
	EPU < 4 NM (7408 m): 1 (0.00%)
	EPU > 10 NM or Unknown: 219 (0.69%)

NACp for ECAT: 'None' records 23576 (35.86%):
	none: 21433 (90.91%)
	EPU < 10 m, VEPU < 15 m: 707 (32.99%)
	EPU < 30 m, VEPU < 45 m: 669 (31.22%)
	EPU < 0.05 NM (93 m): 536 (25.01%)
	EPU < 0.1 NM (185 m): 19 (0.89%)
	EPU > 10 NM or Unknown: 212 (9.89%)

NACp for ECAT: 'light aircraft <= 15500 lbs' records 1624 (2.47%):
	none: 3 (0.18%)
	EPU < 10 m, VEPU < 15 m: 1360 (83.90%)
	EPU < 30 m, VEPU < 45 m: 179 (11.04%)
	EPU < 0.05 NM (93 m): 18 (1.11%)
	EPU < 0.1 NM (185 m): 50 (3.08%)
	EPU > 10 NM or Unknown: 14 (0.86%)

NACp for ECAT: 'rotocraft' records 318 (0.48%):
	none: 0 (0.00%)
	EPU < 10 m, VEPU < 15 m: 280 (88.05%)
	EPU > 10 NM or Unknown: 38 (11.95%)
--------------------------------------------------------------------------------
```

## Questions & Request

For additional information or request for additional categories / editions / analysis scripts please contact the author.

## Author
Helmut Puhr
Contact: compass@openats.at

## Licenses
The source code is released under [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)

The binary is released under [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/), [Legal Text](https://creativecommons.org/licenses/by/4.0/legalcode)

Disclaimer
----------

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
