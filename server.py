from flask import Flask, request, json, render_template
import sqlite3
from datetime import datetime, timedelta
import codecs

# Constants
SAMPLE_PERIOD = 2000    # 2 seconds
TIMEZONE_DELTA = timedelta(hours=7)

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
            data = json_data["data"]
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
        rows = [row for row in cur.execute('SELECT * FROM booty')]

        data = list()
        labels = list()
        table_data = list()
        for r in rows:
            received = datetime.strptime(r[0], '%d-%m-%y %H:%M:%S') - TIMEZONE_DELTA
            row_data = list()
            for i in range(0,len(r[4]),2):
                spd = int(r[4][i:i+2], 16)
                row_data.append(spd)
                data.append(spd)
                calc_datetime = received - timedelta(milliseconds=SAMPLE_PERIOD*(len(r[4]) - i - 1))
                labels.append(calc_datetime.strftime('%m/%d %H:%M:%S'))

            table_data.append((received.strftime('%m/%d/%Y %H:%M:%S'), r[1], r[2], r[3], round(sum(row_data)/len(row_data),3), max(row_data), min(row_data)))

        table_data.sort(key=(lambda r: r[0]))
        headers = ("Time", "Latitude", "Longitude", "Accuracy (km)", "Avg (mph)", "Min (mph)", "Max (mph)")

        if len(table_data) == 1:
            rows.append(["No data present."])
        con.close()
        return render_template('home.html', headers=headers, rows=table_data, data=json.dumps(data), labels=json.dumps(labels))

    return None
