# AI Usage Documentation

## Tool used
Google Gemini (gemini.google.com)

## What I asked for
1. A function `parse_condition(const char *input, char *field, char *op, char *value)`
   that splits a string of the form `field:operator:value` into its three parts.

2. A function `match_condition(Report *r, const char *field, const char *op, const char *value)`
   that returns 1 if a report record satisfies the condition and 0 otherwise.

I provided the AI with the report struct definition and explained the supported
fields (severity, category, inspector, timestamp) and operators (==, !=, <, <=, >, >=).

## What was generated
Both functions were generated as shown in the code. `parse_condition` uses `strchr()`
to locate the two colons and copies each segment with `strncpy`. `match_condition`
dispatches by field name using `strcmp`, handles severity and timestamp as integers,
and category and inspector as strings. The AI correctly used `atol()` for timestamp
to handle 64-bit time_t values.

## What I changed and why
- The AI used `r->category` and `r->inspector` as field names in `match_condition`,
  but my struct uses `issue` and `name` respectively. I updated these to match
  my actual struct definition.
- The AI used `Report` as the struct type name, while my code defines it as
  `REPORT_T`. I updated the function signature accordingly.

## What I learned
- `strchr()` returns a pointer into the original string, so pointer arithmetic
  can extract substrings without extra allocations.
- The AI generated mostly correct code but used generic field names that didn't
  match my actual struct — always verify generated code against your own definitions.
- The AI correctly handled the `atol()` vs `atoi()` distinction for timestamp,
  which shows it's worth reading generated code carefully rather than assuming
  it's wrong.