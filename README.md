 Intelligent Access Verification System (BST)

A C++ access control system for a high-security biomedical lab that uses only binary search trees to store researchers, requests and logs, and shows the tree live in the browser through WebAssembly.

**Live website:** [https://nidhishreeg53-star.github.io/Lab-Access-Verification-System/](https://nidhishreeg53-star.github.io/Lab-Access-Verification-System/)

 Features
- Researchers are stored in a binary search tree keyed by researcher ID, drawn live on the page with the search path highlighted.
- Pending access requests and the access log are also binary search trees.
- Five checks for every request: authentication, authorization, permission, revocation and emergency lockdown.
- Inorder, preorder and postorder traversals, tree size and height.
- Add, revoke, deactivate and delete researchers.

Files
| File | Purpose |
|---|---|
| `access_tree.cpp` | All the logic: binary search tree, verification and a console menu |
| `index.html` | The web page that draws the tree and calls the C++ code |
| `.github/workflows/pages.yml` | Compiles the C++ to WebAssembly and publishes the site on GitHub Pages |

