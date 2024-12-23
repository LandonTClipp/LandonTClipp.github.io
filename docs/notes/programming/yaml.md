---
title: YAML
---

YAML... Yet another markup language. YAML Ain't a Markup Language!

Explicit Mappings
-----------------

Mappings in YAML can be defiend either as implicit, such as:

```yaml
foo: bar
one: two
```

Or using the explicit notation:

```yaml
?foo
: bar
?one
: two
```

Why ever use the explicit notation? Well, you can use any kind of yaml structure as the key:

```yaml
%YAML 1.2
---
? - Detroit Tigers
  - Chicago cubs
: - 2001-07-23
```
<div class="result">
```title=""
>>> from ruamel.yaml import YAML
>>> yaml=YAML()
>>> yaml.load("""%YAML 1.2
... ---
... ? - Detroit Tigers
...   - Chicago cubs
... : - 2001-07-23""")
{('Detroit Tigers', 'Chicago cubs'): [datetime.date(2001, 7, 23)]}
```
</div>

Note that this requires a parser capable of loading YAML 1.2. As of 2023-12-20, the more popular `yaml` and `PyYaml` packages don't support this syntax.
