import json

with open('cmake-build-debug/prices.json', 'r') as f:
    data = json.load(f)

lines = []
for symbol, info in data.items():
    # Filter out meta fields like "price" or "time"
    if symbol.lower() in ["price", "time"]:
        continue
    
    # Ensure info is a dict and has a "price" key
    if isinstance(info, dict) and "price" in info:
        price = info["price"]
        lines.append(f'        prices["{symbol}"] = {price};')

# Sort lines for better organization
lines.sort()

with open('static_prices_updated.txt', 'w') as f:
    f.write("\n".join(lines))
