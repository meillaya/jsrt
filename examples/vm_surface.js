let arr = [1, 2, 3];
let obj = {name: "old", nested: {value: arr[0]}};

arr[2] = 7;

let i = 0;
let total = 0;
do {
  i = i + 1;
  if (i == 2) {
    continue;
  }
  total = total + i;
} while (i < 4);

switch (total) {
  case 8:
    obj.name = "new";
    break;
  default:
    obj.name = "bad";
}

obj["nested"].value = arr[2];
arr[0] + ":" + obj.name + ":" + total + ":" + obj.nested.value;
