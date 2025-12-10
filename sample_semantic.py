import requests
from sentence_transformers import SentenceTransformer, util

class StellarTraceSemantic:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url.rstrip('/')
        self.model = SentenceTransformer("all-MiniLM-L6-v2")
        self.docs = []
        self.emb = None

    def load_docs(self):
        r = requests.get(f"{self.base_url}/all")
        data = r.json()
        self.docs = [d.get("content", "") for d in data]
        self.emb = self.model.encode(self.docs, convert_to_tensor=True)

    def search(self, query, top_k=5):
        q = self.model.encode(query, convert_to_tensor=True)
        scores = util.pytorch_cos_sim(q, self.emb)[0]
        idx = scores.topk(top_k).indices
        return [self.docs[i] for i in idx]

if __name__ == "__main__":
    s = StellarTraceSemantic()
    s.load_docs()

    while True:
        q = input("Search: ").strip()
        if q == "":
            break
        print("Results:", s.search(q))
