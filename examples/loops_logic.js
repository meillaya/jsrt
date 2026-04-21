let i = 0;
let total = 0;
let meta = {name: "jsrt", active: true};

while (i < 4 && meta.active) {
    total = total + i;
    i = i + 1;
}

(total == 6 && meta.name) || "fallback";
