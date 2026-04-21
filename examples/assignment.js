let arr = [1, 2, 3];
let obj = {name: "old", nested: {value: arr[0]}};

arr[2] = 7;
obj.name = "new";
obj["nested"].value = arr[2];

arr[0] + ":" + obj.name + ":" + obj.nested.value;
