let values = [1, 2];
let alias = values;

function touch(items) {
  items[1] = 9;
  return items;
}

let returned = touch(alias);
returned[0] = 5;

let obj = {nested: {value: 1}};
let nested = obj.nested;
nested.value = 7;

values[0] + ":" + values[1] + ":" + obj.nested.value;
