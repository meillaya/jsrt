let out = 0;

switch (2) {
  case 1:
    out = 1;
    break;
  case 2:
    out = out + 2;
  case 3:
    out = out + 3;
    break;
  default:
    out = 99;
}

out;
