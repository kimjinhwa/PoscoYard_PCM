#include <BluetoothSerial.h>
#include "bluetoothtask.h"
#include "WiFi.h"
#include "SimpleCLI.h"
#include "EEPROM.h"
#include "readAmpere.h"
#include "../../src/main.h"
#include "../../version.h"
SimpleCLI simpleCli;
BluetoothSerial SerialBT;

void readInputSerialBT();
void simpleCliSetup();
void reboot_Callback(cmd *c){
    SerialBT.printf("Rebooting...\n");
    ESP.restart();
}
void version_Callback(cmd *c){
    String ver = String(VERSION);
    SerialBT.printf("Ver: %s\n", ver.c_str());
}
void id_Callback(cmd *cmdPtr){
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String id = arg.getValue();
    SerialBT.printf("Modbus ID : %d\n", nvmSet.ModbusAddress);
}
void cells_Callback(cmd *cmdPtr){
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String cells = arg.getValue();
    if(cells.length() > 0){
        SerialBT.printf("Current Installed Cells : %d\n", nvmSet.InstalledCells);
        nvmSet.InstalledCells = cells.toInt();
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
        EEPROM.commit(); //Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
        SerialBT.printf("New Installed Cells : %d\n", nvmSet.InstalledCells);
    }
    else{
        SerialBT.printf("Installed Cells : %d\n", nvmSet.InstalledCells);
    }

}
void ssid_Callback(cmd *cmdPtr){
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String ssid = arg.getValue();
    if(ssid.length() > 0){
        if(ssid.compareTo("erase") == 0){
            strncpy(nvmSet.SSID, "", sizeof(nvmSet.SSID));
            strncpy(nvmSet.PASS, "", sizeof(nvmSet.PASS));
            EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
            EEPROM.commit(); //Commit EEPROM
            EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
            SerialBT.printf("SSID erased\n");
        }
        else{
            SerialBT.printf("Current SSID : %s\n", nvmSet.SSID);
            strncpy(nvmSet.SSID, ssid.c_str(), sizeof(nvmSet.SSID));
            EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
            EEPROM.commit(); //Commit EEPROM
            EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
            SerialBT.printf("New SSID : %s\n", nvmSet.SSID);
        }
    }
    else{
        SerialBT.printf("SSID : %s\n", nvmSet.SSID);
    }
}
void pass_Callback(cmd *cmdPtr){
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String pass = arg.getValue();
    if(pass.length() > 0){
        SerialBT.printf("Current PASS : %s\n", nvmSet.PASS);
        strncpy(nvmSet.PASS, pass.c_str(), sizeof(nvmSet.PASS));
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
        EEPROM.commit(); //Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
        SerialBT.printf("New PASS : %s\n", nvmSet.PASS);
    }
    else{
        SerialBT.printf("PASS : %s\n", nvmSet.PASS);
    }

}
void relayControl_Callback(cmd *cmdPtr){
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String relayValue = arg.getValue();
    if(relayValue.length() > 0){
        relayControl.setRelayStatus(relayValue.toInt());
        SerialBT.printf("RELAY STATUS : %d\n", relayControl.readRelayStatus());
    }
    else{
        SerialBT.printf("RELAY STATUS : %d\n", relayControl.readRelayStatus());
    }

}
void again_Callback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String gain = arg.getValue();
    if(gain.length() > 0){
        SerialBT.printf("Current GAIN : %d\n", nvmSet.AmpereGain);
        nvmSet.AmpereGain = gain.toInt();
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
        EEPROM.commit(); //Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
        SerialBT.printf("OFFSET : %d\n", nvmSet.AmpereOffset);
        SerialBT.printf("Current [AMPERE] : %2.2f\n", _ReadAmpereClass.ampereAverage);
        SerialBT.printf("New GAIN : %d\n", nvmSet.AmpereGain);

    }

    else{
        SerialBT.printf("GAIN : %d\n", nvmSet.AmpereGain);
        SerialBT.printf("OFFSET : %d\n", nvmSet.AmpereOffset);
        SerialBT.printf("Current [AMPERE] : %2.2f\n", _ReadAmpereClass.ampereAverage);
    }

}
void maxChargeAmpere_Callback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String maxChargeAmpere = arg.getValue();
    if(maxChargeAmpere.length() > 0){
        SerialBT.printf("Current MAX CHARGE AMPERE : %d\n", nvmSet.CutOffChargeAmpere);
        nvmSet.CutOffChargeAmpere = maxChargeAmpere.toInt();
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
        EEPROM.commit(); //Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
        SerialBT.printf("New MAX CHARGE AMPERE : %d\n", nvmSet.CutOffChargeAmpere);
    }
    else{
        SerialBT.printf("MAX CHARGE AMPERE : %d\n", nvmSet.CutOffChargeAmpere);
    }
}

void aoffset_Callback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String offset = arg.getValue();
    if(offset.length() > 0){
        SerialBT.printf("Current OFFSET : %d\n", nvmSet.AmpereOffset);
        nvmSet.AmpereOffset = offset.toInt();
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
        EEPROM.commit(); //Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
        SerialBT.printf("GAIN : %d\n", nvmSet.AmpereGain);
        SerialBT.printf("Current [AMPERE] : %2.2f\n", _ReadAmpereClass.ampereAverage);
        SerialBT.printf("New OFFSET : %d\n", nvmSet.AmpereOffset);
    }
    else{
        SerialBT.printf("OFFSET : %d\n", nvmSet.AmpereOffset);
        SerialBT.printf("GAIN : %d\n", nvmSet.AmpereGain);
        SerialBT.printf("Current [AMPERE] : %2.2f\n", _ReadAmpereClass.ampereAverage);
    }

}
void minDischargeAmpere_Callback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String offset = arg.getValue();
    if (offset.length() > 0)
    {
        SerialBT.printf("Current MIN DISCHARGE AMPERE : %d\n", nvmSet.CutOffDischargeAmpere);
        nvmSet.CutOffDischargeAmpere = offset.toInt();
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); // Write to EEPROM
        EEPROM.commit();                                             // Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));  // Read from EEPROM
        SerialBT.printf("New MIN DISCHARGE AMPERE : %d\n", nvmSet.CutOffDischargeAmpere);
    }
    else
    {
        SerialBT.printf("MIN DISCHARGE AMPERE : %d\n", nvmSet.CutOffDischargeAmpere);
    }
}
void UseHoleCTRatio_Callback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument arg = cmd.getArgument(0);
    String UseHoleCTRatio = arg.getValue();
    if(UseHoleCTRatio.length() > 0){
        SerialBT.printf("Current USE HOLE CT : %d\n", nvmSet.UseHoleCTRatio);
        //이 값을 CT Tatio로 변경하기 위하여 변경한다
        nvmSet.UseHoleCTRatio = UseHoleCTRatio.toInt() ;

        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
        EEPROM.commit(); //Commit EEPROM
        EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
        SerialBT.printf("GAIN : %d\n", nvmSet.AmpereGain);
        SerialBT.printf("OFFSET : %d\n", nvmSet.AmpereOffset);
        SerialBT.printf("Current [AMPERE] : %2.2f\n", _ReadAmpereClass.ampereAverage);
    }
    else{
        SerialBT.printf("Current USE HOLE CT : %d\n", nvmSet.UseHoleCTRatio);
        SerialBT.printf("GAIN : %d\n", nvmSet.AmpereGain);
        SerialBT.printf("OFFSET : %d\n", nvmSet.AmpereOffset);
        SerialBT.printf("Current [AMPERE] : %2.2f\n", _ReadAmpereClass.ampereAverage);
    }
}
void help_Callback(cmd *c){
    SerialBT.printf("%s\n", simpleCli.toString().c_str());
}
void error_Callback(cmd_error *e){
    CommandError cmdError(e); // Create wrapper object
    SerialBT.print("ERROR: ");
    SerialBT.println(cmdError.toString());
    if (cmdError.hasCommand()) {
        SerialBT.print("Did you mean \"");
        SerialBT.print(cmdError.getCommand().toString());
        SerialBT.println("\"?");
    }
}
void ip_Callback(cmd *cmdPtr){
    if(WiFi.isConnected()){
        SerialBT.printf("IPAddress: %s\n", WiFi.localIP().toString().c_str());
        SerialBT.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        SerialBT.printf("SubnetMask: %s\n", WiFi.subnetMask().toString().c_str());
    }
    else{
        SerialBT.printf("Not connected\n");
    }
}
Command reboot;
Command cmdversion;
Command id;
Command cells;
Command ssid;
Command pass;
Command relayCommand;
Command again;
Command aoffset;
Command maxChargeAmpere_command;
Command minDischargeAmpere_command;
Command UseHoleCTRatio;
Command help;
Command ip;
void simpleCliSetup()
{
    reboot = simpleCli.addCommand("reboot", reboot_Callback);
    reboot.setDescription("Reboot the device");
    cmdversion = simpleCli.addCommand("ver/sion", version_Callback);
    cmdversion.setDescription("Get the version");
    id = simpleCli.addSingleArgCmd("id", id_Callback);
    id.setDescription("Set the Modbus ID");
    cells = simpleCli.addSingleArgCmd("cells", cells_Callback);//InstalledCells
    cells.setDescription("Set the Installed Cells");
    ssid = simpleCli.addSingleArgCmd("ssid", ssid_Callback);//SSID
    ssid.setDescription("Set the SSID");
    pass = simpleCli.addSingleArgCmd("pass", pass_Callback);//PASS
    pass.setDescription("Set the PASS");
    relayCommand = simpleCli.addSingleArgCmd("relay", relayControl_Callback);//REF
    relayCommand.setDescription("Set the Relay Control");
    again = simpleCli.addSingleArgCmd("again", again_Callback);//GAINo
    again.setDescription("Set the Ampere GAIN");
    aoffset = simpleCli.addSingleArgCmd("aoff/set", aoffset_Callback);//OFFSET
    aoffset.setDescription("Set the Ampere OFFSET");
    maxChargeAmpere_command = simpleCli.addSingleArgCmd("max/ChargeAmpere", maxChargeAmpere_Callback);//GAINo
    maxChargeAmpere_command.setDescription("Set the Max Charge Ampere");
    minDischargeAmpere_command = simpleCli.addSingleArgCmd("min/Ampere", minDischargeAmpere_Callback);//OFFSET
    minDischargeAmpere_command.setDescription("Set the Min Discharge Ampere");
    UseHoleCTRatio = simpleCli.addSingleArgCmd("use/current", UseHoleCTRatio_Callback);//USE HOLE CT
    UseHoleCTRatio.setDescription("Set the USE HOLE CT");
    ip = simpleCli.addSingleArgCmd("ip", ip_Callback);//IP
    ip.setDescription("Read the IPAddress,GW,SUBNETMASK");
    help = simpleCli.addCommand("help",help_Callback); 
    help.setDescription("Get help!");
    simpleCli.setOnError(error_Callback);
}
void bluetoothTaskSetup(void)
{
    String bleName = "PcmBms";
    bleName += String(WiFi.macAddress());
    bleName += "_";
    bleName += String(nvmSet.ModbusAddress);
    bleName.toLowerCase();
    Serial.printf("\nBluetooth Name : %s\n", bleName.c_str());

    SerialBT.begin(bleName.c_str());
    simpleCliSetup();
}
void bluetoothTask(void *pvParameters)
{
    bluetoothTaskSetup();
    for(;;)
    {
        readInputSerialBT();
        vTaskDelay(1);
    }
}
String inputString;
void readInputSerialBT()
{
  char readBuf[2];
  char readCount = 0;
  // while (true)
  {
    if (SerialBT.available())
    {
      if (SerialBT.readBytes(readBuf, 1))
      {
        readBuf[1] = 0x00;
        if (readBuf[0] == 8)
        {
          inputString.remove(inputString.length() - 1);
        }
        else
        {
          printf("%c", readBuf[0]);
          inputString += String(readBuf);
        }
      }
      if (readBuf[0] == '\n' || readBuf[0] == '\r')
      {
        simpleCli.parse(inputString);
        while (SerialBT.available()) SerialBT.readBytes(readBuf, 1);
        inputString = "";
        SerialBT.printf("\n# ");
        // break;
      }
    }
  }
}