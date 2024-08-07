# README #

Private toy project

![screenshot7](images/ss7.png)
![screenshot6](images/ss6.png)
![screenshot5](images/ss5.png)
![screenshot4](images/ss4.png)
![screenshot3](images/ss3.png)
![screenshot2](images/ss2.png)
![screenshot](images/ss.png)

# What is this repository for? ##

Just playing around...

# Build HOWTO #

## Windows ##

### Build and run client ###

1. Install Visual Studio 2022 Community (with C++ support)
2. Install CMake
3. Clone this repository on `C:\laidoff`
4. Open command prompt
5. Execute: `cd c:\laidoff && mkdir build && cd build && cmake ..`
6. Open `client.sln` generated at `C:\laidoff\build`
7. Rebuild all
8. Set `laidoff` as StartUp Project
9. Run!

### Build and run server ###

#### Build `laidoff-server` (written in C) ####

1. Proceed steps decribed in "How do I get set up? (Client on Windows)"
2. Set `laidoff-server` as StartUp Project
3. Run!

### Make client connect to localhost ###

1. Open `c:\laidoff\assets\conf\conf.json`
2. Find & Replace `p.popsongremix.com` to `localhost`
3. Run client

## Linux ##

### `laidoff-server` ###

1. Execute: `mkdir build-server && cd build-server`
2. Execute: `cmake .. -DSERVER_ONLY=1`
3. Execute: `make`

## Diagrams ##

![matchsequence](images/matchsequence.png)
