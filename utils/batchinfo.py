def read_batchinfo(filename):
    d = {}
    with open(filename) as f:
        for line in f:
            line = line.strip("\"").strip("\n")
            if '=' in line:
                (key, val) = line.split("=")
                try:
                    d[key.strip(" ")] = int(val)
                except ValueError:
                    try:
                        d[key.strip(" ")] = float(val)
                    except ValueError:
                        d[key.strip(" ")] = val
    return d
