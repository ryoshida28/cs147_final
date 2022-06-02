from flask import Flask
from flask import request
from flask import render_template
import sqlite3

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

            # Store Data
            con = sqlite3.connect('booty.db')
            cur = con.cursor()
            cur.execute("CREATE TABLE IF NOT EXISTS booty(datetime text, latitude real, longitude real, accuracy real, data real)")
            cur.execute("INSERT INTO booty VALUES(?, ?, ?, ?, ?)", (time, lat, lon, cep, data))
            con.commit()
            con.close()

            print(f"time: {time}\tlatitude: {lat}\tlongitude: {lon}\tcep: {cep}\tdata: {data}", flush=True)
            return f"time: {time}\tlatitude: {lat}\tlongitude: {lon}\tcep: {cep}\tdata: {data}"
        print("No json data.")
    
    # Display data on page
    if request.method == "GET":
        con = sqlite3.connect('booty.db')
        cur = con.cursor()
        rows = [row for row in cur.execute('SELECT * FROM booty ORDER BY ROWID DESC')]
        rows.insert(0, ["Time", "Latitude", "Longitude", "Accuracy", "Data"])
        if len(rows) == 1:
            rows.append(["No data present."])
        con.close()
        return render_template('home.html', rows=rows)

    return None
