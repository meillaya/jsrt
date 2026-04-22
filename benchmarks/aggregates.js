let values = [1, 2, 3, 4];
let meta = {name: "jsrt", total: 0, nested: {last: values[0]}};
let i = 0;
while (i < 80) {
  values[0] = values[0] + 1;
  values[3] = values[3] + values[0];
  meta.total = meta.total + values[3];
  meta.nested.last = values[0];
  i = i + 1;
}
meta.name + ":" + meta.total + ":" + meta.nested.last;
