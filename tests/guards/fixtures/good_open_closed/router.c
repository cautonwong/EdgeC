// Compliant: small switch or table-driven dispatch
int handle_msg(int type) {
    switch (type) {
        case 1:
            return 10;
        case 2:
            return 20;
        default:
            return 0;
    }
}
