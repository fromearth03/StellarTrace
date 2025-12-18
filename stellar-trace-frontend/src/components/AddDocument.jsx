import { useState } from "react";

export default function AddDocument({ onClose }) {
  const [authors, setAuthors] = useState([{ first: "", last: "" }]);

  const addAuthor = () => {
    setAuthors([...authors, { first: "", last: "" }]);
  };

  const removeAuthor = (i) => {
    if (authors.length === 1) return;
    setAuthors(authors.filter((_, idx) => idx !== i));
  };
  const handleAddDocument = async () => {
    const inputs = document.querySelectorAll(".input");

    const [
      titleInput,
      abstractInput,
      submitterInput,
      categoriesInput,
      fullRefInput,
      doiInput,
      licenseInput,
      updateDateInput,
      commentsInput
    ] = inputs;

    const title = titleInput.value.trim();
    const abstract = abstractInput.value.trim();

    if (!title || !abstract) {
      alert("Title and Abstract are required.");
      return;
    }

    const authorRows = document.querySelectorAll(
      "div[style*='display: flex'][style*='gap: 10px']"
    );

    const authors_parsed = [];

    authorRows.forEach(row => {
      const fields = row.querySelectorAll("input");
      if (fields.length >= 2) {
        const first = fields[0].value.trim();
        const last = fields[1].value.trim();
        if (first || last) {
          authors_parsed.push([last, first, ""]);
        }
      }
    });

    if (authors_parsed.length === 0) {
      alert("At least one author is required.");
      return;
    }

    const payload = {
      title,
      abstract,
      authors_parsed,
      submitter: submitterInput?.value || "",
      categories: categoriesInput?.value || "",
      full_publication_reference: fullRefInput?.value || "",
      digital_object_identifier_full_text: doiInput?.value || "",
      license: licenseInput?.value || "",
      update_date: updateDateInput?.value || "",
      comments: commentsInput?.value || ""
    };

    try {
      const res = await fetch("http://localhost:8080/adddoc", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });

      if (!res.ok) throw new Error();

      alert("Document added successfully.");
    } catch {
      alert("Failed to add document.");
    }
  };


  return (
    <div style={{ background: "#f5f6f8", height: "100vh", overflow: "auto", padding: "40px" }}>
      {/* Page Heading */}
      <h1 style={{ textAlign: "center", color: "#1e293b", fontSize: "28px", fontWeight: "700", marginBottom: "30px" }}>
        Add Document
      </h1>

      <div
        style={{
          maxWidth: "900px",
          margin: "0 auto",
          background: "white",
          borderRadius: "8px",
          boxShadow: "0 2px 10px rgba(0,0,0,0.08)",
          padding: "30px",
          marginBottom: "40px"
        }}
      >
        <h2 style={{ textAlign: "center", marginBottom: "25px", color: "#333" }}>
          Document Details
        </h2>

        {/* TITLE */}
        <label>Title <span style={{ color: "red" }}>*</span></label>
        <input className="input" />

        {/* ABSTRACT */}
        <label style={{ marginTop: "20px" }}>
          Abstract <span style={{ color: "red" }}>*</span>
        </label>
        <textarea rows={5} className="input" />

        {/* AUTHORS */}
        <div style={{ marginTop: "20px" }}>
          <div style={{ display: "flex", justifyContent: "space-between" }}>
            <label>
              Authors <span style={{ color: "red" }}>*</span>
            </label>
            <button type="button" onClick={addAuthor} className="link">
              + Add Author
            </button>
          </div>

          {authors.map((a, i) => (
            <div key={i} style={{ display: "flex", gap: "10px", marginTop: "8px" }}>
              <input placeholder="First Name" className="input" />
              <input placeholder="Last Name" className="input" />
              {authors.length > 1 && (
                <button
                  type="button"
                  onClick={() => removeAuthor(i)}
                  className="remove"
                >
                  ✕
                </button>
              )}
            </div>
          ))}
        </div>

        {/* SUBMITTER */}
        <label style={{ marginTop: "20px" }}>Submitter</label>
        <input className="input" />

        {/* CATEGORIES */}
        <label style={{ marginTop: "20px" }}>Categories</label>
        <input placeholder="e.g. cs.AI, cs.IR" className="input" />

        {/* FULL PUBLICATION REFERENCE */}
        <label style={{ marginTop: "20px" }}>
          Full Publication Reference
        </label>
        <input className="input" />

        {/* DOI */}
        <label style={{ marginTop: "20px" }}>
          Digital Object Identifier (full text)
        </label>
        <input className="input" />

        {/* LICENSE */}
        <label style={{ marginTop: "20px" }}>License</label>
        <input className="input" />

        {/* UPDATE DATE */}
        <label style={{ marginTop: "20px" }}>Update Date</label>
        <input type="date" className="input" />

        {/* COMMENTS */}
        <label style={{ marginTop: "20px" }}>Comments</label>
        <textarea rows={3} className="input" />

        {/* ACTIONS */}
        <div
          style={{
            display: "flex",
            justifyContent: "flex-end",
            gap: "10px",
            marginTop: "30px"
          }}
        >
          <button className="btn-secondary" onClick={onClose}>Cancel</button>
          <button className="btn-primary" onClick={handleAddDocument}>
            Add Document
          </button>
        </div>
      </div>

      {/* INLINE STYLES (KEEP SIMPLE) */}
      <style>{`
        label {
          display: block;
          margin-bottom: 4px;
          font-weight: 500;
          color: #333;
        }
        .input {
          width: 100%;
          padding: 8px 10px;
          border: 1px solid #ccc;
          border-radius: 4px;
          color: #000;
        }
        .btn-primary {
          background: #2563eb;
          color: white;
          border: none;
          padding: 8px 16px;
          border-radius: 4px;
          cursor: pointer;
          transition: background 0.2s, transform 0.1s;
        }
        .btn-primary:hover {
          background: #1d4ed8;
        }
        .btn-primary:active {
          background: #1e40af;
          transform: scale(0.97);
        }
        .btn-secondary {
          background: #f1f5f9;
          border: 1px solid #94a3b8;
          padding: 8px 16px;
          border-radius: 4px;
          color: #334155;
          cursor: pointer;
        }
        .btn-secondary:hover {
          background: #e2e8f0;
        }
        .link {
          background: none;
          border: none;
          color: #2563eb;
          cursor: pointer;
        }
        .remove {
          background: none;
          border: none;
          color: #888;
          cursor: pointer;
        }
      `}</style>
    </div>
  );
}
