let total = 0;
for (let i = 0; i < 5; i = i + 1) {
  if (i == 2) {
    continue;
  }
  total = total + i;
}

let limited = 0;
for (let i = 0; ; i = i + 1) {
  if (i == 4) {
    break;
  }
  limited = limited + i;
}

total + ":" + limited;
