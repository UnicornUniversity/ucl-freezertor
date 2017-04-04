# Freezertor 1.0

Demo application demonstrating usage of BigClown its selected modules and Unicorn Application Framework. Temporarly Grafana and Influx DB are used.

## Influx DB

Database: __uclFreezertor__

### Schemas

| Schema      | Columns                 |
| ----------- | ----------------------- |
| temperature | time, node, temp        |
| illuminance | time, node, illuminance |

### Query

curl -G 'http://localhost:8086/query?pretty=true' --data-urlencode "db=uclFreezertor" --data-urlencode "q=SELECT temp FROM temperature"

### Start
launchctl load ~/Library/LaunchAgents/homebrew.mxcl.influxdb.plist
