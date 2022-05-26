from flask import Flask
from flask import request

app = Flask(__name__)

@app.route("/", methods=['POST', 'GET'])
def getData():
    # data: string 50 chars (1 byte each)
    # integer MPH over each interval (6 seconds)
    # JSON params: transmit_time (UTC), iridium_latitude, iridium_longitude, iridium_cep (est accuracy of lat/long), data

    # Update data from satellite
    if request.method == "POST":
        json_data = request.get_json()
        if json_data:
            time = json_data["transmit_time"]
            lat = json_data["iridium_latitude"]
            lon = json_data["iridium_longitude"]
            cep = json_data["iridium_cep"]
            data = bytes.fromhex(json_data["data"]).decode("utf-8")
            print(f"time: {time}\tlatitude: {lat}\tlongitude: {lon}\tcep: {cep}\tdata: {data}", flush=True)
            return f"time: {time}\tlatitude: {lat}\tlongitude: {lon}\tcep: {cep}\tdata: {data}"
        print("No json data.")
    
    # Display data on page
    if request.method == "GET":
        return """
<table>
  <tr>
    <th>Time</th>
    <th>Latitude</th>
    <th>Longitude</th>
    <th>Accuracy</th>
    <th>Data</th>
  </tr>
  <tr>
    <td>This is time</td>
    <td>Alfreds Futterkiste</td>
    <td>Maria Anders</td>
    <td>Germany</td>
    <td>some data</td>
  </tr>
</table>
"""
    return None