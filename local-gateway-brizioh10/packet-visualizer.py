import sys
import serial
import json
import gpiod
import time
from lcd import lcd


def xtract_json_from_string(strjson):
	
	retstr=""
	lensj = len(strjson)
	#print("lensj : %d" % lensj)
	if lensj != 0:
		pS=strjson.find("{")
		if pS>-1:
			pE=strjson.find("}",pS)
			#print("pS: %d, pE: %d" % pS, pE)
			#print('{0:2d},{1:2d}'.format(pS,pE))
			if pE>-1:
				retstr = strjson[pS:pE+1]
				#print("retstr : %s" % retstr)
				if lensj>pE+1: 
					strjson = strjson[pE+1:]
				else:
					strjson = ""
			else:
				retstr =""
		else:
			strjson = ""
			retstr =""
	else:
		retstr =""

	return retstr, strjson

def report_ds18b20_node(dictH10):
	if "temperature" in dictH10:
		#t = float(dictH10["temperature"]) / 100.
		print("temperature : %5.2f" % dictH10["temperature"])

def report_senseair_node(dictH10):
	if "conc_ppm" in dictH10:
		print("conc_ppm    : %5d" % dictH10["conc_ppm"])
	if "temperature" in dictH10:
		print("temperature : %5.2f" % dictH10["temperature"])


def report_lis2dw12_node(dictH10):
	if "acc_x" in dictH10:
		print("acc_x       : %d" % dictH10["acc_x"])
	if "acc_y" in dictH10:
		print("acc_y       : %d" % dictH10["acc_y"])
	if "acc_x" in dictH10:
		print("acc_z       : %d" % dictH10["acc_z"])
	if "temperature" in dictH10:
		print("temperature : %d" % dictH10["temperature"])
	if "pitch" in dictH10:
		print("pitch       : %5.2f" % dictH10["pitch"])
	if "roll" in dictH10:
		print("roll        : %5.2f" % dictH10["roll"])
	if "yaw" in dictH10:
		print("yaw         : %5.2f" % dictH10["yaw"])
		
def report_ht_node(dictH10, cfg_dict):
	lcd_stringT = ""
	lcd_stringH = ""
	lcd_string = ""
	lcd_local = "  "
	if "temperature" in dictH10:
		print("temperature :%s" % dictH10["temperature"])
		lcd_stringT = str(dictH10["temperature"]) + "C"
		lcd_stringT = lcd_stringT.strip()

	if "humidity" in dictH10:
		print("humidity    :%s" % dictH10["humidity"])
		lcd_stringH = str(dictH10["humidity"]) +"%"
		lcd_stringH = lcd_stringH.strip()

	if cfg_dict["lcd_enabled"] == 1:
		cpuid = dictH10["cpuid"]
		if cpuid == cfg_dict["HT_LOCAL_CPU_ID"]:
			winstar.setcurpos(0,0)
			lcd_local = " L "
		else:
			winstar.setcurpos(0,1)
			lcd_local = " E "
		lcd_string = lcd_stringT + lcd_local + lcd_stringH
		winstar.putstring(lcd_string)


def report_bme688_node(dictH10, cfg_dict):
	lcd_string = ""
	lcd_local = "  "
	if "temperature" in dictH10:
		print("temperature :%s" % dictH10["temperature"])
	if "humidity" in dictH10:
		print("humidity    :%s" % dictH10["humidity"])
	if "pressure" in dictH10:
		print("pressure    :%s" % dictH10["pressure"])
	if "gas" in dictH10:
		print("gas         :%s" % dictH10["gas"])


def report_cmt_node(dictH10, cfg_dict):
	retblnAlert = False
	if "alert" in dictH10:
		print("alert       :%s" % dictH10["alert"])
		if dictH10["alert"] != "0000":
			retblnAlert = True
			if "cpuid" in dictH10:
				cpuid = dictH10["cpuid"]
				if cfg_dict["CMT1_ENABLED"]:
					if cpuid == cfg_dict["CMT1_CPU_ID"]:
						print("ALERT 1 - ALERT 1 - ALERT 1")
						lines_1.set_values([1])
						time.sleep(cfg_dict["CMT1_TIME_ON"])
						lines_1.set_values([0])

				if cfg_dict["CMT2_ENABLED"]:
					if cpuid == cfg_dict["CMT2_CPU_ID"]:
						print("ALERT 2 - ALERT 2 - ALERT 2")
						lines_2.set_values([1])
						time.sleep(cfg_dict["CMT2_TIME_ON"])
						lines_2.set_values([0])
	return retblnAlert


def report_no_sensor_node(dictH10, cfg_dict):
	retblnAlert = True

	return retblnAlert


def report_node_header(dictH10):

	if "rssi" in dictH10:
		print("rssi        : %5d" % dictH10["rssi"])
	if "snr" in dictH10:
		print("snr         : %5d" % dictH10["snr"])
	if "signature" in dictH10:
		print("signature   :  %s" % dictH10["signature"])
	if "s_class" in dictH10:
		print("s_class     : %5d" % dictH10["s_class"])
	if "cpuid" in dictH10:
		print("cpuid       : %s" % dictH10["cpuid"])
	if "vcc" in dictH10:
		print("vcc         : %5.3f" % dictH10["vcc"])
	if "vpanel" in dictH10:
		print("vpanel      : %5.3f" % dictH10["vpanel"])
	if "node_power" in dictH10:
		print("node_power  : %5d" % dictH10["node_power"])		
	if "node_boost" in dictH10:
		print("node_boost  : %5d" % dictH10["node_boost"])
	if "sleep_time" in dictH10:
		print("sleep_time  : %5d" % dictH10["sleep_time"])			

		
def report_node_type(strjson, cfg_dict):
	
	nodeclass = {
		"0"  : "NO Sensor",
		"1"  : "HDC3020  - Humidity Temperature",
		"2"  : "LIS2DW12 - Accelerometer Temperature",
		"3"  : "SENSEAIR - CO2 Temperature",
		"4"  : "D",
		"5"  : "CiccioMeTocca",
		"6"  : "BME688   - Humidity Temperature Pressure Gas",
		"7"  : "DS18B20  - Temperature"
	}
	dictH10 = dict()
	lensj = len(strjson)
	#print("check_CMT_node lensj : %d" % lensj)
	#print("check_CMT_node strjson : %s" % strjson)
	ctime = time.time()
	ltime = time.localtime(ctime)
	ms = int((ctime - int(ctime)) * 1000)
	s_time = "@ {0:02d}:{1:02d}:{2:02d}.{3:03d}".format(ltime.tm_hour, ltime.tm_min, ltime.tm_sec,ms)	
	if lensj != 0:
		#strjson ="{\"cpuid\":\"5C659F99463953512020203110050E02\",\"class\":5}"
		#strjson ="{\"rssi\":-68,\"snr\":5,\"signature\":\"00e0\",\"s_class\":5,\"cpuid\":\"5C659F99463953512020203110050E02\",\"vcc\":\"3.285\",\"vpanel\":\"0.026\",\"alert\":\"0000\"}"
		dictH10 = json.loads(strjson)
		if "s_class" in dictH10:
			s_class = dictH10["s_class"]
			if((s_class>=0)and(s_class<=7)):
				s_strclass = "{0:d}".format(s_class)
				strclass = nodeclass[s_strclass]
				print('{0:s} Node {1:s}'.format(strclass, s_time))
				retblnAlert = report_node_header(dictH10)
				match(s_class):
					case 0:
						report_no_sensor_node(dictH10, cfg_dict)
					case 1:
						report_ht_node(dictH10, cfg_dict)
					case 2:
						report_lis2dw12_node(dictH10)
					case 3:
						report_senseair_node(dictH10)
					case 4:
						print('D Node INFO')
					case 5:
						report_cmt_node(dictH10, cfg_dict)
					case 6:
						report_bme688_node(dictH10, cfg_dict)
					case 7:
						report_ds18b20_node(dictH10)
					case _:
						print('UNKOWN Node {0:s}'.format(s_time))
			else:
				print("NOT An Acme Node : Wrong Class {0:3d}".format(s_class))
		else:
			
			pS=strjson.find("blob")
			if pS>-1:
				print("Unkown Node / Info : Blob Received")
				print("{0:s}".format(strjson))
			else:
				print("NOT An Acme Node : Missing Class")
	else:
		print("NOT An Acme Node : Empy String")

#######################################################



print("\n\nStarting FoxD27 - Gateway\n" )
with open("/home/acme/local-gateway-brizioh10/brizio-H10.json", "r") as fp:
	cfg_dict = json.load(fp)  

#
#	check configuration and define defaults if necessary
#
if "lcd_enabled" in cfg_dict:
	print("Cfg : lcd_enabled is set as  %d" % cfg_dict["lcd_enabled"])
else:
	cfg_dict["lcd_enabled"] = 0
	print("Cfg : lcd_enabled default to %d" % cfg_dict["lcd_enabled"])

if "CMT1_ENABLED" in cfg_dict:
	print("Cfg : CMT1_ENABLED is set as  %d" % cfg_dict["CMT1_ENABLED"])
	if cfg_dict["CMT1_ENABLED"] == 1:
		if "CMT1_CPU_ID" in cfg_dict:
			print("Cfg : CMT1_CPU_ID is set as  %s" % cfg_dict["CMT1_CPU_ID"])
			if "CMT1_TIME_ON" in cfg_dict:
				print("Cfg : CMT1_TIME_ON is set as  %f" % cfg_dict["CMT1_TIME_ON"])
			else:
				cfg_dict["CMT1_TIME_ON"] = 0.05
				print("Cfg : CMT1_TIME_ON default to %f" % cfg_dict["CMT1_TIME_ON"])
		else:
			cfg_dict["CMT1_ENABLED"] = 0
			print("Cfg : CMT1_CPU_ID missing, CMT1_ENABLED default to %d" % cfg_dict["CMT1_ENABLED"])
else:
	cfg_dict["CMT1_ENABLED"] = 0
	print("Cfg : CMT1_ENABLED default to %d" % cfg_dict["CMT1_ENABLED"])

if "CMT2_ENABLED" in cfg_dict:
	print("Cfg : CMT2_ENABLED is set as  %d" % cfg_dict["CMT2_ENABLED"])
	if cfg_dict["CMT2_ENABLED"] == 1:
		if "CMT2_CPU_ID" in cfg_dict:
			print("Cfg : CMT2_CPU_ID is set as  %s" % cfg_dict["CMT2_CPU_ID"])
			if "CMT2_TIME_ON" in cfg_dict:
				print("Cfg : CMT2_TIME_ON is set as  %f" % cfg_dict["CMT2_TIME_ON"])
			else:
				cfg_dict["CMT2_TIME_ON"] = 0.05
				print("Cfg : CMT2_TIME_ON default to %f" % cfg_dict["CMT2_TIME_ON"])
		else:
			cfg_dict["CMT2_ENABLED"] = 0
			print("Cfg : CMT2_CPU_ID missing, CMT2_ENABLED default to %d" % cfg_dict["CMT2_ENABLED"])
else:
	cfg_dict["CMT2_ENABLED"] = 0
	print("Cfg : CMT2_ENABLED default to %d" % cfg_dict["CMT2_ENABLED"])

if "lcd_enabled" in cfg_dict:
	print("Cfg : lcd_enabled set to %d" % cfg_dict["lcd_enabled"])
	if cfg_dict["lcd_enabled"] == 1:
		try:
			winstar = lcd(2)
			winstar.clear()
			winstar.putstring(" FOX Board D27")
			winstar.setcurpos(0,1)
			winstar.putstring("  BRIZIO-H10")
		except :
			print("Error Opening LCD, LCD is now Disabled\n")
			cfg_dict["lcd_enabled"] = 0
else:
	cfg_dict["lcd_enabled"] = 0
	print("Cfg : lcd_enabled default to %d" % cfg_dict["lcd_enabled"])


if "HT_LOCAL_ENABLED" in cfg_dict:
	print("Cfg : HT_LOCAL_ENABLED set to %d" % cfg_dict["HT_LOCAL_ENABLED"])
	if cfg_dict["HT_LOCAL_ENABLED"] == 1:
		if cfg_dict["HT_LOCAL_CPU_ID"] != "":
			print("Cfg : HT_LOCAL_CPU_ID is set as  %s" % cfg_dict["HT_LOCAL_CPU_ID"])
		else:
			cfg_dict["HT_LOCAL_ENABLED"] = 0
			print("Cfg : HT_LOCAL_CPU_ID missing ")
			print("--->  HT_LOCAL_ENABLED default to %d" % cfg_dict["HT_LOCAL_ENABLED"])
else:
	cfg_dict["HT_LOCAL_ENABLED"] = 0
	print("Cfg : HT_LOCAL_ENABLED default to %d" % cfg_dict["HT_LOCAL_ENABLED"])



if "serial_port" in cfg_dict:
	print("Cfg : serial_port is set as  %s" % cfg_dict["serial_port"])
else:
	cfg_dict["serial_port"] = "/dev/ttyS1"
	print("Cfg : serial_port default to %s" % cfg_dict["serial_port"])
	
#
#	Opening serial port
#
ser = serial.Serial(
	port=cfg_dict["serial_port"],
	baudrate=115200,
	timeout=.1,
	parity=serial.PARITY_NONE,
	stopbits=serial.STOPBITS_ONE,
	bytesize=serial.EIGHTBITS
)

linecounter=0
bytecounter=0
bytesperline = 8
bufferjson =""

#bufferjson ="{prova stringa}"
#bufferjson ="{prova str"
#bufferjson =" stringa}"
#bufferjson ="{prova stringa}{inizio"
#bufferjson ="{prova stringa}{seconda chiave}"
#bufferjson = "{\"rssi\":-68,\"snr\":5,\"signature\":00e0,\"s_class\":5,\"cpuid\":\"5C659F99463953512020203110050E02\",\"vcc\":3.285,\"vpanel\":0.026,\"alert\":0000}"
#bufferjson = "{\"cpuid\":\"5C659F99463953512020203110050E02\",\"class\":5}"
#jsonBH10,bufferjson = xtract_json_from_string(bufferjson)
#print("1 jsonBH10 :%s" % jsonBH10)
#print("1 bufferjason :%s\n" % bufferjson)
#check_CMT_node(jsonBH10)
#jsonBH10,bufferjson = xtract_json_from_string(bufferjson)
#print("2 jsonBH10 :%s" % jsonBH10)
#print("2 bufferjson :%s\n" % bufferjson)
#check_CMT_node(jsonBH10)


#bufferjson = "{\"rssi\":-68,\"snr\":5,\"signature\":00e0,\"s_class\":5,\"cpuid\":\"5C659F99463953512020203110050E02\",\"vcc\":3.285,\"vpanel\":0.026,\"alert\":0000}"
#bufferjson = bufferjson.replace("00e0","\"00e0\"")
#print("3 bufferjson :%s\n" % bufferjson)
#exit()

chip=gpiod.Chip('gpiochip0')
line_1 = gpiod.find_line("PB25")
lines_1 = chip.get_lines([line_1.offset()])
lines_1.request(consumer='foobar', type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
lines_1.set_values([0])

line_2 = gpiod.find_line("PB24")
lines_2 = chip.get_lines([line_2.offset()])
lines_2.request(consumer='foobar', type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
lines_2.set_values([0])

while True:
	bytesToRead = ser.inWaiting()
	if bytesToRead>0:
		value=ser.read(bytesToRead)
		bufferjson = bufferjson + value.decode("utf-8")
		sys.stdout.flush()
		jsonBH10,bufferjson = xtract_json_from_string(bufferjson)
		if len(jsonBH10)>0:
			print("\n\nReceived Packet :")
			print("%s\n" % jsonBH10)
			report_node_type(jsonBH10, cfg_dict)
	else:
		time.sleep(.1)
