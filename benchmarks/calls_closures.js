function makeCounter(start) {
  let current = start;
  return function (step) {
    current = current + step;
    return current;
  };
}

let counter = makeCounter(2);
let total = 0;
let step = 1;
for (let i = 0; i < 60; i = i + 1) {
  total = total + counter(step);
  if (step == 4) {
    step = 1;
  } else {
    step = step + 1;
  }
}
total;
