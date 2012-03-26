The ART+COM AG Watchdog is a utility to start, watch and control applications and computer. It has a interface to control its functionality via udp-network packets. 
It is open-source and available for windows, linux and mac-osx. 

# Checkout
Checkout watchdog sources and initiate submodules like this:
 
    git clone git@github.com:artcom/watchdog.git
    cd watchdog
    git submodule init  
    git submodule update  


# Build
### Windows
#### Prerequisits
- Visual Studio Express 9 2008, 32Bit
- CMAKE Version 2.8 or higher
- PRO60 Dependencies: [[https://y60.artcom.de/redmine/attachments/download/146/PRO60Dependencies-1.0.6-win32.exe]]

#### Build process
Create build target directory:
  
    mkdir obj  
    cd obj  

Make build scripts using cmake (either for nmake or visual Studio 9 2008).  
Build with ide: 

    cmake -G "Visual Studio 9 2008" .. 
    open Watchdog.sln and build it using the ide

Build with nMake via shell: 

    cmake -G "NMake Makefiles" ..
    nmake

### Linux
#### Install dependencies
The Ubuntu way:

     sudo apt-get install build-essential autoconf2.13 cmake libboost-dev libglib2.0-dev libcurl4-openssl-dev libasound2-dev

#### Create yourself a build directory (you will need one per build configuration)

    mkdir -p _builds/release

#### Configure the build tree (this is the equivalent of ./configure)

    cd _build/release
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local ../../

#### Build the sources

    make -jX

### Mac

# Usage
The commandline usage: i.e. with windows executable:

    'watchdog.exe [--configfile <watchdog.xml>] [--help] [--no_restart]'  
  
The default configfile will be loaded in current working directoy with name 'watchdog.xml', the parameter '--configfile <watchdog.xml>' will use given configfile.  
The common watchdog behaviour is to restart application once they exit by any reason, to prevent this use the '--no_restart' flag, which will exit the watchdog after application finish.

# Configuration

All the functions are configured via the xml-configfile, which elements will be explained in detail below.


The smallest possible configuration is this:

    <WatchdogConfig logfile="watch.log" watchFrequency="15">
       <Application binary="calc.exe"/>
    </WatchdogConfig>

It will start the application 'calc.exe' check its status every 15 seconds and log all watchdog output to the logfile 'watch.log'.

The full feature set is divided in three categories:  
1. The application startup configuration, timed restart commands and runtime checks, like memory cunsumption and heartbeat detecction.  
2. Systemcommand pre and post application execution and timed computer restart or shutdown   
3. Udpcontrol interface for status and controlling of computer, application and a bunch of projectors (will be updated to a plugin infrastructure)  

All optional nodes are obsolete, the functionality will be disabled.
The notation ${env_var} will evaluate the environment variable 'env_var' into the string, and is used in heartbeat_file definition, 
application binary,arguments and working directory.

The feature set in Detail:

### Application execution

Full featured application configuration node:

    <Application binary="calc.exe" directory="">
        <EnvironmentVariables>
            <!--<EnvironmentVariable name="key"><![CDATA[value]]></EnvironmentVariable>-->
        </EnvironmentVariables>
        <Arguments>
            <Argument>test.txt</Argument>
        </Arguments>
        <<Heartbeat>
            <Heartbeat_File>${TEMP}/heartbeat.xml</Heartbeat_File>
            <Allow_Missing_Heartbeats>5</Allow_Missing_Heartbeats>
            <Heartbeat_Frequency>1</Heartbeat_Frequency>
            <FirstHeartBeatDelay>120</FirstHeartBeatDelay>
        </Heartbeat>
        <WaitDuringStartup>0</WaitDuringStartup>
        <WaitDuringRestart>10</WaitDuringRestart>                
        <Memory_Threshold>100000</Memory_Threshold>
        <RestartDay>Monday</RestartDay>
        <RestartTime>12:02</RestartTime>
        <CheckMemoryTime>00:00</CheckMemoryTime>
        <CheckTimedMemoryThreshold>150000</CheckTimedMemoryThreshold>
    </Application>
    
- The <Application> root-node defines the app to execute and watch
  Attributes: <binary>    - defines the binary filename
              <directory> - watchdog changes to this directory before executing app [optional]

Optional nodes:              

- \<Arguments> defines a list of application arguments
  and has children of <Argument>-nodes with childnode definition of the environment variable
  with the use of CDATA-definition it is possible to handle specials character easier (i.e. '\').
  
- <EnvironmentVariables> defines a list of environment variables, that will be set before app executing
  and has children of <EnvironmentVariable>-nodes with key-value definition of the environment variable
  with the use of CDATA-definition it is possible to handle specials character easier (i.e. '\').

- The Heartbeat detection will check the content of a given file and expects the seconds since 1970 in a format like this:
<heartbeat secondsSince1970="1332154365"/>
  The childnode <Heartbeat_File> will define the heartbeat file in its childnode. 
  The childnode <Allow_Missing_Heartbeats> will define the allowed mssing heartbeat before the watchdog assumes a app to be dead.
  The childnode <Heartbeat_Frequency> will define expected frequency ot the heartbeat in seconds.
  The childnode <FirstHeartBeatDelay> will define delay in seconds, before the heartbeat detection begins (i.e. for a longer app starttime).

- <WaitDuringStartup> will define a startup time before the app starts
- <WaitDuringRestart> will define a wait time before the app restarts
- <RestartTime> will define a time string, at which the apps restarts
- <RestartDay> will define a day on which the <RestartTime> will be used to restart the app
- <Memory_Threshold> will define a threshold of free system memory, the undercut of the threshold will lead to a  app restart
- <CheckMemoryTime> will define a threshold of free system memory which will be used with node <CheckTimedMemoryThreshold> to check memory
   assumption at a given timestamp.
- <CheckTimedMemoryThreshold> will define a timestamp at which memory-assumption threshold from node <CheckMemoryTime >will be used to check 


### Systemcommand execution 

Full featured systemcommand configuration node:

    <RebootTime>15:30</RebootTime>
    <HaltTime>16:10</HaltTime>
    <AppPreTerminateCommand>
		<![CDATA[dir]]>    
    </AppPreTerminateCommand>    
    <AppTerminateCommand ignoreOnUdpRestart="false">
		<![CDATA[dir]]>    
    </AppTerminateCommand>    
    <PreShutdownCommand>
		<![CDATA[dir]]>    
    </PreShutdownCommand>
    <PreStartupCommand>
		<![CDATA[dir]]>    
    </PreStartupCommand>

Fully Optional nodes:            
  
- <RebootTime> will define a timestamp at which the computer will reboot
- <HaltTime> will define a timestamp at which the computer will shutdown
- <AppPreTerminateCommand> will define a systemcommand to be executed before the app will be terminated
- <AppTerminateCommand> will define a systemcommand to be executed after the app terminated
   Attributes: <ignoreOnUdpRestart> - can be made dependent if the app is restartet via udp or exited internally
- <PreShutdownCommand> will define a systemcommand to be executed before the computer is shutdown
- <PreStartupCommand> will define a systemcommand to be executed when the watchdog is started

### Udpcontrol interface

Full featured Udpcontrol configuration node:

    <UdpControl port="2346">
        <IpWhitlelist>
            <Ip>10.1.3.91</Ip>
            <Ip>127.0.0.1</Ip>
        </IpWhitlelist>
        <SystemHalt powerDownProjectors="true" command="halt"/>    
        <SystemReboot shutterCloseProjectors="true" command="reboot"/>
        <RestartApplication command="restart_app"/>
        <StopApplication shutterCloseProjectors="true" command="firealarm_on"/>
        <StartApplication command="firealarm_off"/>
        <ProjectorControl powerUpOnStartup="true">
            <Projector type="nec" port="0" baud="38400" input="RGB_1"/>
        </ProjectorControl>
        <StatusReport command="status" loadingtime="2"/>
        <ContinuousStatusChangeReport ip="10.1.1.106" port="6655"/>
    </UdpControl>

- The <UdpControl> root-node enabled the udp control functionality of the watchdog
  Attributes: <port>    - port the watchdog will listen to

Optional:  
- <IpWhitlelist> defines a list of ip-adresses, for which the watchdog allows udp control
  and has children of <Ip>-nodes that define thw whitelist. If defined only senderhost with the 
  configured ip will be accepted.

- <SystemHalt> will add the listener to the specified command, if a udp-packet with this content is accepted, the computer
  will shutdown

- <SystemReboot> will add the listener to the specified command, if a udp-packet with this content is accepted, the computer will reboot

- <RestartApplication> will add the listener to the specified command, if a udp-packet with this content is accepted, the watchdog will restart the application

- <StopApplication> will add the listener to the specified command, if a udp-packet with this content is accepted, the watchdog will stop the application

- <StartApplication> will add the listener to the specified command, if a udp-packet with this content is accepted, the watchdog  will start the application

- <StatusReport> will add the listener to the specified command, if a udp-packet with this content is accepted, the watchdog sends back the status of the application [running, loading, terminated]. If the attribute <loadingtime> in seconds is specified, the status  loading will be send for the given amount of seconds to simulate loading procedure

- <ContinuousStatusChangeReport> triggers a continuous status change udp command to given ip and port

- The node <ProjectorControl> holds projector control nodes. The feature will be rewritten, therefore not documented, its usage experimental

