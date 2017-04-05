# Unicorn College Repository ucl-freezertor

## Description

Demo application demonstrating usage of BigClown and its selected modules, wireless comuniation and an integration with Unicorn Application Framework.

> Grafana and Influx DB are used temporarly.

## Hardware

- 2x BigClown Core Module
- 1x BigClown Climate Module
- 1x BigClown Battery Module
- 1x BigClown Base Module
- 1x Raspberry Pi

### Base Station

- 1x BigClown Core Module
- 1x BigClown Base Module

### Remote Station

- 1x BigClown Core Module
- 1x BigClown Climate Module
- 1x BigClown Battery Module

### Field Gateway

- 1x Raspberry Pi

## Influx DB

Database: __uclFreezertor__

### Tables

| Tables      | Columns                 |
| ----------- | ----------------------- |
| temperature | time, node, temp        |
| illuminance | time, node, illuminance |