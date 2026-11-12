let fns = {};
for (let i = 0; i < 3; i = i + 1) {
    fns["k" + i] = function () { return i; };
}

let order = [0, 0];
let index = 0;
order[index] = (index = 1) + 10;

let escapes = "\u0041\x42";
let object = {class: 1, default: 2,};
let duplicate = {a: 1, a: 2};

fns["k0"]() + ":" + fns["k1"]() + ":" + fns["k2"]() + "|" +
    order[0] + ":" + order[1] + "|" +
    escapes + ":" + object.class + ":" + object.default + "|" +
    duplicate.a + ":" + len(duplicate) + "|" +
    regexMatch("abc123", "[a-z]+[0-9]+");
