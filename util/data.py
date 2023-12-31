import csv
import numpy as np
from datetime import datetime
from constants.constants import NORMALIZED_DATA, API_OUTPUT_FILE, TEMP_N, PRESSURE_N, DEWPOINT_N, HUMIDITY_N, LUMINOSITY_N


def read_data(path: str) -> ([], []):
    with open(path, "r") as file:
        reader = csv.DictReader(file)
        header = list(next(reader))
        data = [list(row.values()) for row in reader]
    return data, header


def prepare_data(data, feature_indices, target_index: int) -> (np.array, np.array):
    x = np.array([[float(row[i]) for i in feature_indices] for row in data])
    y = np.array([float(row[target_index]) for row in data])
    return x, y


def to_label(number) -> int:
    try:
        number = float(number)
    except TypeError:
        return -1
    if number == 0:
        return 0
    if number < 0.5:
        return 1
    if number < 4:
        return 2
    if number < 8:
        return 3
    return 4


def normalize_data(input_path: str, output_path: str) -> bool:
    try:
        with open(input_path, "r") as file:
            reader = csv.DictReader(file)
            data = list(reader)
    except FileNotFoundError:
        return False

    data = sorted(data, key=lambda x: datetime.strptime(x["time"], "%Y-%m-%dT%H:%M"))

    try:
        for i in range(len(data)):
            data[i]["precipitation_1h"] = (
                to_label(data[i + 1]["precipitation"]) if i < len(data) - 1 else 0.0
            )
            data[i]["precipitation_6h"] = (
                to_label(data[i + 6]["precipitation"]) if i < len(data) - 6 else 0.0
            )
            data[i]["precipitation_12h"] = (
                to_label(data[i + 12]["precipitation"]) if i < len(data) - 12 else 0.0
            )
            data[i]["precipitation_24h"] = (
                to_label(data[i + 24]["precipitation"]) if i < len(data) - 24 else 0.0
            )
            data[i]["temperature_2m"] = float(data[i]["temperature_2m"]) / TEMP_N
            data[i]["dew_point_2m"] = float(data[i]["dew_point_2m"]) / DEWPOINT_N
            data[i]["relative_humidity_2m"] = (
                float(data[i]["relative_humidity_2m"]) / HUMIDITY_N
            )
            data[i]["surface_pressure"] = float(data[i]["surface_pressure"]) / PRESSURE_N
            data[i]["shortwave_radiation"] = (
                float(data[i]["shortwave_radiation"]) / LUMINOSITY_N
            )
    except (ValueError, IndexError):
        print("Data not valid")
        return False

    fields = data[0].keys()

    try:
        with open(output_path, "w") as file:
            writer = csv.DictWriter(file, fieldnames=fields)
            writer.writeheader()
            writer.writerows(data)
    except FileNotFoundError:
        return False
    return True


def normalize_sensors(data):
    try:
        normalized = {"temperature_2m": float(data["temperature"]) / TEMP_N, "dew_point_2m": float(data["dewpoint"]) / DEWPOINT_N,
                      "relative_humidity_2m": float(data["humidity"]) / HUMIDITY_N,
                      "surface_pressure": float(data["pressure"]) / PRESSURE_N,
                      "shortwave_radiation": float(data["luminosity"]) / LUMINOSITY_N / 4000}
        return normalized
    except (ValueError, IndexError):
        print("Data not valid")
        return False


def main():
    if not normalize_data(API_OUTPUT_FILE, NORMALIZED_DATA):
        print("Could not normalize data")


if __name__ == "__main__":
    main()
