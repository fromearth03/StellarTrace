import requests

class StellarTraceAutoComplete:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url.rstrip('/')

    def suggest(self, prefix, max_results=10):
        params = {'q': prefix}
        try:
            resp = requests.get(f"{self.base_url}/search", params=params, timeout=2)
            data = resp.json()
            results = []
            for item in data:
                title = item.get('title')
                if title:
                    results.append(title)
                if len(results) >= max_results:
                    break
            return results
        except:
            return []

if __name__ == "__main__":
    ac = StellarTraceAutoComplete()
    print("Type search queries (enter blank to exit)")
    while True:
        txt = input("Search: ").strip()
        if txt == "":
            break
        print("Suggestions:", ac.suggest(txt))
