// VIOLATION: Giant monolithic hardcoded switch dispatch (> 12 cases) violating OCP
int handle_msg(int type) {
    switch (type) {
        case 1:
            return 10;
        case 2:
            return 20;
        case 3:
            return 30;
        case 4:
            return 40;
        case 5:
            return 50;
        case 6:
            return 60;
        case 7:
            return 70;
        case 8:
            return 80;
        case 9:
            return 90;
        case 10:
            return 100;
        case 11:
            return 110;
        case 12:
            return 120;
        case 13:
            return 130;
        case 14:
            return 140;
        default:
            return 0;
    }
}
