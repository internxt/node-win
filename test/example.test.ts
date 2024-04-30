function sumar(a: number, b: number): number { return a + b; }


test('suma 1 + 2 igual a 3', () => {
  expect(sumar(1, 2)).toBe(3);
});
