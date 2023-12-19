import sys
import serial
import json
import gpiod
import time
import paho.mqtt.publish as publish

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

def load_data(name, label, dictH10, retdic):
	
	if name in dictH10:
		value = dictH10[name]
		print("{0:12s} : {1:s}".format(name, value))
		retdic[label] = value
	
	return retdic	
	

def report_ds18b20_node(dictH10, retdic):
	
	load_data("temperature", 'T', dictH10, retdic)
	
	return retdic
	
def report_senseair_node(dictH10, retdic):
	
	load_data("conc_ppm", 'c', dictH10, retdic)
	load_data("temperature", 't', dictH10, retdic)
		
	return retdic

def report_lis2dw12_node(dictH10, retdic):

	load_data("acc_x", 'x', dictH10, retdic)
	load_data("acc_y", 'y', dictH10, retdic)
	load_data("acc_z", 'z', dictH10, retdic)
	load_data("temperature", 't', dictH10, retdic)
	load_data("pitch", 'p', dictH10, retdic)
	load_data("roll", 'r', dictH10, retdic)
	load_data("yaw", 'Y', dictH10, retdic)

	return retdic
		
def report_hdc3020_node(dictH10, retdic):

	load_data("temperature", 't', dictH10, retdic)
	load_data("humidity", 'h', dictH10, retdic)	

	return retdic

def report_bme688_node(dictH10, retdic):
	
	load_data("temperature", 't', dictH10, retdic)
	load_data("humidity", 'h', dictH10, retdic)
	load_data("pressure", 'p', dictH10, retdic)
	load_data("gas", 'g', dictH10, retdic)

	return retdic

def report_cmt_node(dictH10, retdic):
	
	load_data("alert", 't', dictH10, retdic)

	return retdic


def report_no_sensor_node(dictH10, retdic):

	return retdic


def report_lis2dw12_ds18b20_node(dictH10, retdic):

	load_data("acc_x", 'x', dictH10, retdic)
	load_data("acc_y", 'y', dictH10, retdic)
	load_data("acc_z", 'z', dictH10, retdic)
	load_data("temp_lis2dw12", 't', dictH10, retdic)
	load_data("pitch", 'p', dictH10, retdic)
	load_data("roll", 'r', dictH10, retdic)
	load_data("yaw", 'Y', dictH10, retdic)
	load_data("tem_ds18b20", 'T', dictH10, retdic)
	
	return retdic
	
def report_node_header(dictH10):
    
	cpu_id = ""
	if "rssi" in dictH10:
		print("rssi        : %5d" % dictH10["rssi"])
	if "snr" in dictH10:
		print("snr         : %5d" % dictH10["snr"])
	if "signature" in dictH10:
		print("signature   :  %s" % dictH10["signature"])
	if "s_class" in dictH10:
		print("s_class     : %5d" % dictH10["s_class"])
	if "cpuid" in dictH10:
		cpu_id = dictH10["cpuid"]
		print("cpuid       : %s" % cpu_id)
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

	return cpu_id

def transmit_data(cpu_id, retdic):
	
	for key in retdic:
		micic = "/brizioh10/" + cpu_id + "/" + key
		msg =  retdic[key]
		print("topic {0}, msg {1}".format(micic, msg))
		publish.single(topic=micic, payload=msg, hostname=host)

def report_node_type(strjson, cfg_dict):
	
	nodeclass = {
		"0"  : "NO Sensor",
		"1"  : "HDC3020  - Hum + Temp",
		"2"  : "LIS2DW12 - Acc + Incl + Temp",
		"3"  : "SENSEAIR - CO2 + Temp",
		"4"  : "D",
		"5"  : "CiccioMeTocca",
		"6"  : "BME688   - Hum + Temp + Press + Gas",
		"7"  : "DS18B20  - Temp",
		"8"  : "LIS2DW12-DS18B20 - Acc + Incl + Temp + Temp"
	}
	return_val = ""
	dictH10 = dict()
	retdic = dict()
	lensj = len(strjson)
	ctime = time.time()
	ltime = time.localtime(ctime)
	ms = int((ctime - int(ctime)) * 1000)
	s_time = "@ {0:02d}:{1:02d}:{2:02d}.{3:03d}".format(ltime.tm_hour, ltime.tm_min, ltime.tm_sec,ms)	
	if lensj != 0:
		dictH10 = json.loads(strjson)
		if "s_class" in dictH10:
			s_class = dictH10["s_class"]
			if((s_class>=0)and(s_class<len(nodeclass))):
				s_strclass = "{0:d}".format(s_class)
				strclass = nodeclass[s_strclass]
				print('{0:s} Node {1:s}'.format(strclass, s_time))
				cpu_id = report_node_header(dictH10)
				match(s_class):
					case 0:
						retdic = report_no_sensor_node(dictH10, retdic)
					case 1:
						retdic = report_hdc3020_node(dictH10, retdic)
					case 2:
						retdic = report_lis2dw12_node(dictH10, retdic)
					case 3:
						retdic = report_senseair_node(dictH10, retdic)
					case 4:
						print('D Node INFO')
					case 5:
						retdic = report_cmt_node(dictH10, retdic)
					case 6:
						retdic = report_bme688_node(dictH10, retdic)
					case 7:
						retdic = report_ds18b20_node(dictH10, retdic)
					case 8:
						retdic = report_lis2dw12_ds18b20_node(dictH10,retdic)						
					case _:
						print('UNKOWN Node {0:s}'.format(s_time))
                
				if(len(retdic) !=0 and cpu_id !=""):
					transmit_data(cpu_id, retdic)
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
with open("brizioh10_publisher.json", "r") as fp:
    cfg_dict = json.load(fp)

#
#	check configuration and define defaults if necessary
#
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




bufferjson =""
host = "localhost"

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
