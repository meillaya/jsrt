function makeAccumulator(seed) {
  let total = {value: seed};
  return function(step) {
    total.value = total.value + step;
    return [{box: {value: total.value}}, total.value + 1];
  };
}

let acc = makeAccumulator(1);
let sum = 0;
for (let i = 0; i < 5; i = i + 1) {
  let pair = acc(i + 1);
  sum = sum + pair[0].box.value + pair[1];
}
sum;
