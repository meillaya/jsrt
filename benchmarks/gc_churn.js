function makeAccumulator(seed) {
  let total = {value: seed};
  return function (step) {
    total.value = total.value + step;
    return [{box: {value: total.value}}, total.value + 1];
  };
}

let acc = makeAccumulator(1);
let sum = 0;
let step = 1;
for (let i = 0; i < 60; i = i + 1) {
  let pair = acc(step);
  sum = sum + pair[0].box.value + pair[1];
  if (step == 5) {
    step = 1;
  } else {
    step = step + 1;
  }
}
sum;
