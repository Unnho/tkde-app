import re, sys

def make_strtol(expr, fallback):
    return f'char* _end=nullptr; long _val=std::strtol({expr}.c_str(),&_end,10); return (_end[0]==\'\\0\'?static_cast<int>(_val):{fallback});'

for path in sys.argv[1:]:
    with open(path) as f: content = f.read()
    orig = content

    # Replace "try { return std::stoi(X); } catch (...) { return Y; }"
    content = re.sub(
        r'try\s*\{\s*return\s+std::stoi\(([^)]+)\);\s*\}\s*catch\s*\(\.\.\.)\s*\{\s*return\s+([^;]+);\s*\}',
        lambda m: f'return {make_strtol(m.group(1), m.group(2))}',
        content
    )

    # Replace "try { X; } catch (...) {}" and "try { X; } catch (...) { Y; }"
    content = re.sub(
        r'try\s*\{([^}]+?)\}\s*catch\s*\(\.\.\.)\s*\{([^}]*?)\}',
        r'\1',
        content
    )

    if content != orig:
        with open(path, 'w') as f: f.write(content)
        print(f'Fixed: {path}')
