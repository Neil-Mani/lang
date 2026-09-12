# Alang

Alang is a small programming language for exploring mathematics and physics problems. It combines familiar programming features with built-in numerical math, vectors, matrices, statistics, calculus, and equation solving.

## Quick Start

Build the interpreter with GCC and Make:

```powershell
make
```

Run an Alang program:

```powershell
.\alang.exe .\examples\combined_problem.alang
```

The interpreter only accepts files with the `.alang` extension.

## Example

```alang
var launch_speed = 50;
var gravity = 9.81;
var target_distance = 150;

var range_error(angle) = ((launch_speed ^ 2) * sin(2 * angle) / gravity) - target_distance;
var solutions = solve(range_error, 0.01, (PI / 2) - 0.01, 2000);

output("Launch angles:");
output(deg(solutions[0]));
output(deg(solutions[1]));
```

This finds the two launch angles that allow a projectile to reach a target 150 meters away.

## Language Features

### Variables and expressions

```alang
var x = 10;
var y = 3;
var result = (x ^ 2) + y;
output(result);
```

Supported operators include:

- Arithmetic: `+`, `-`, `*`, `/`, `^`
- Comparisons: `==`, `<`, `>`
- Logic: `&&`, `||`
- Assignment: `=`

### Conditions and loops

```alang
if (x > 5) {
    output("large");
} else {
    output("small");
}

for (var i = 0; i < 3; i = i + 1) {
    output(i);
}

while (x > 0) {
    x = x - 1;
}
```

### Functions and equations

Regular functions use `fn`:

```alang
fn add(a, b) {
    return a + b;
}

output(add(2, 3));
```

Equations use function-style `var` declarations:

```alang
var cubic(x) = x ^ 3;
output(cubic(2));
```

### Input and output

```alang
var name = input("Name: ");
var age = input_number("Age: ");
output("Hello, " + name);
output(age);
```

`output()` prints numbers, strings, arrays, and nested arrays.

## Math Functions

### General math

```alang
abs(x)
sqrt(x)
cbrt(x)
pow(x, y)
exp(x)
ln(x)
log(x)
log10(x)
log2(x)
floor(x)
ceil(x)
round(x)
min(x, y, ...)
max(x, y, ...)
```

`log()` and `ln()` both calculate the natural logarithm.

### Trigonometry

```alang
sin(x)
cos(x)
tan(x)
asin(x)
acos(x)
atan(x)
atan2(y, x)
rad(degrees)
deg(radians)
```

Trigonometric functions use radians. The constants `PI`, `E`, and `TAU` are available:

```alang
output(deg(PI / 2));
output(sin(rad(90)));
```

### Statistics

Statistical functions accept a non-empty numeric array:

```alang
var values = [1, 2, 2, 3, 4];
output(sum(values));
output(product(values));
output(mean(values));
output(median(values));
output(mode(values));
output(variance(values));
output(stddev(values));
```

`variance()` uses population variance. When multiple values tie for the mode, the smallest value is returned.

### Vectors

Vectors are numeric arrays. They can be created with `vector()` or written as array literals:

```alang
var a = vector(1, 0, 0);
var b = [0, 1, 0];

output(magnitude(a));
output(normalize([3, 4]));
output(dot(a, b));
output(cross(a, b));
output(angle_between(a, b));
```

`cross()` currently requires three-dimensional vectors. `angle_between()` returns radians.

### Matrices

Matrices are nested numeric arrays:

```alang
var A = matrix([[1, 2], [3, 4]]);

output(det(A));
output(transpose(A));
output(inverse(A));
output(eigenvalues(A));
```

`det()` and `inverse()` require square matrices. `transpose()` also supports rectangular matrices. `eigenvalues()` currently supports real 1x1 and 2x2 matrices.

### Calculus

Numerical calculus can operate on named one-argument functions:

```alang
fn square(x) {
    return x * x;
}

output(derivative("square", 3));
output(integral("square", 0, 1));
output(limit("square", 2));
```

Equation variables can also produce symbolic results:

```alang
var cubic(x) = x ^ 3;

output(derivative(cubic));        # (3 * (x ^ 2))
output(integral(cubic));          # ((x ^ 4) / 4) + C
output(derivative(cubic, 2));     # 12
output(integral(cubic, 0, 1));    # 0.25
```

Current symbolic differentiation and integration support common arithmetic expressions, powers, and selected functions such as `sin`, `cos`, and `exp`. Bounded integrals use the trapezoidal rule.

### Roots and equation solving

`root(value, degree)` calculates an nth root:

```alang
output(root(64, 6));
output(root(0 - 8, 3));
```

`solve()` scans an interval for numerical roots and returns an array:

```alang
var quadratic(x) = (x ^ 2) - 4;
var roots = solve(quadratic, 0 - 10, 10);
output(roots);       # [-2, 2]
```

An optional fourth argument controls the number of interval samples:

```alang
solve(quadratic, 0 - 10, 10, 5000)
```

## Examples

- [math.alang](examples/math.alang): broad math, calculus, vectors, matrices, and equations tour
- [projectile.alang](examples/projectile.alang): projectile-motion calculation
- [combined_problem.alang](examples/combined_problem.alang): math functions combined with loops, conditions, arrays, and user-defined equations
- [main.alang](examples/main.alang): general language feature sample

Run an example with:

```powershell
.\alang.exe .\examples\projectile.alang
```

## VS Code Syntax Highlighting

The repository includes a local VS Code language extension in [alang-language-support](alang-language-support). It highlights comments, keywords, functions, builtins, constants, numbers, strings, operators, and punctuation.

To activate it during development:

1. Open the repository in VS Code.
2. Open **Run and Debug** with `Ctrl+Shift+D`.
3. Select **Run Alang syntax extension**.
4. Press `F5`.
5. Open an `.alang` file in the new Extension Development Host window.

## Project Structure

```text
src/
  AST.c
  io.c
  lexer.c
  main.c
  parser.c
  token.c
  visitor.c
  include/
examples/
alang-language-support/
Makefile
```

## Current Limitations

Alang is an active prototype. Current limitations include:

- Dynamic runtime values with limited static type checking
- Numerical root solving can miss roots that do not cross the x-axis
- Symbolic calculus supports a limited set of expression forms
- Eigenvalues are currently limited to real 1x1 and 2x2 matrices
- Matrices and vectors are represented as arrays
- Unary negative syntax such as `-5` is not yet supported; use `0 - 5`
- The local syntax extension currently runs through an Extension Development Host

## License

No license has been selected for this project yet.
