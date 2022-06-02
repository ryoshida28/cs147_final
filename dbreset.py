import sqlite3

con = sqlite3.connect('booty.db')
cur = con.cursor()
cur.execute("DROP TABLE IF EXISTS booty")
cur.execute("CREATE TABLE IF NOT EXISTS booty(datetime text, latitude real, longitude real, accuracy real, data real)")
con.close()

print("Table Dropped.")
