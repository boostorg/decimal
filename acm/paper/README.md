# Boost.Decimal TOMS paper outline

This folder holds the outline of a research paper for ACM Transactions on
Mathematical Software (TOMS). It is a scaffold, not a finished manuscript: each
section lists what it should contain so the prose can be filled in.

## Files

- `toms_decimal.tex` - the paper, as an `acmart` document in the `acmsmall`
  journal format that TOMS uses, with `\acmJournal{TOMS}`.
- `references.bib` - a seed bibliography with the key prior-art references.
- `Makefile` - builds the PDF against the template in `../template` without
  modifying any template file.

## Why a research paper (not an Algorithms paper)

See `../research.adoc`, section "ACM Transactions on Mathematical Software". In
short: the contribution is a whole type system plus a comparative engineering
and performance study, the novelty is portability, constexpr, and GPU rather
than a single new algorithm, and the research-paper route plus the Replicated
Computational Results (RCR) badge fits a living, permissively licensed Boost
library better than depositing a frozen snapshot in CALGO.

## Building

The ACM class files live in `../template` and are left untouched. The Makefile
sets `TEXINPUTS`, `BSTINPUTS`, and `BIBINPUTS` so LaTeX finds `acmart.cls` and
`ACM-Reference-Format.bst` there.

```sh
make        # produces toms_decimal.pdf
make clean
```

A LaTeX installation with the usual ACM dependencies is required. If you build
by hand instead of with make, export the same search paths first, for example:

```sh
export TEXINPUTS=.:../template//:
export BSTINPUTS=.:../template//:
export BIBINPUTS=.:../template//:
pdflatex toms_decimal && bibtex toms_decimal && \
  pdflatex toms_decimal && pdflatex toms_decimal
```

## Before submitting

- Replace every bracketed TODO and the itemized outline with prose.
- Regenerate the CCS concept codes with the ACM CCS tool at
  https://dl.acm.org/ccs and paste the generated block in.
- Fill in the rights, volume, and received-date commands from the ACM rights
  form once the paper is accepted.
- Complete the bibliography entries (page numbers, DOIs) in `references.bib`.
