# Pyhon Distance Testing Code
 
import asyncio 
import time 
import serial 
import winsdk.windows.devices.geolocation as geolocation 
 
async def get_coords(): 
    locator = geolocation.Geolocator() 
    locator.report_interval = 2000 
    position = await locator.get_geoposition_async() 
    latitude = position.coordinate.latitude 
    longitude = position.coordinate.longitude 
    return latitude, longitude 
 
def get_location(): 
    try: 
        return asyncio.run(get_coords()) 
    except PermissionError: 
        print("ERROR: You need to allow applications to access your location in Windows settings.") 
        return (0.0, 0.0) 
 
ser = serial.Serial('COM5', 115200, timeout=1)  # Update 'COM5' if needed 
 
filename = input("Please a filename for output (include .txt): ") 
 
with open(filename, 'w') as log_file: 
    while True: 
        if ser.in_waiting > 0:              try: 
                rssi_data = ser.readline().decode('utf-8').strip() 
                latitude, longitude = get_location() 
                log_entry = f"{time.strftime('%Y-%m-%d %H:%M:%S')} | RSSI: {rssi_data} | Lat: {latitude}, Lon: {longitude}\n" 
                print(log_entry)   
                log_file.write(log_entry)   
 
                log_file.write(f"Google Maps Link: https://maps.google.com/?q={latitude},{longitude}\n") 
                time.sleep(2) 
            except Exception as e: 
                print(f"Error processing serial data: {e}") 
