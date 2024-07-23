#ifndef _AP_hexen_C_DEF_
#define _AP_hexen_C_DEF_

static int is_hexen_type_ap_location(int doom_type)
{
    switch (doom_type)
    {
        case 10:
        case 12:
        case 13:
        case 16:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 30:
        case 32:
        case 33:
        case 36:
        case 53:
        case 84:
        case 86:
        case 123:
        case 8002:
        case 8003:
        case 8005:
        case 8006:
        case 8007:
        case 8008:
        case 8009:
        case 8010:
        case 8030:
        case 8031:
        case 8032:
        case 8033:
        case 8034:
        case 8035:
        case 8036:
        case 8037:
        case 8038:
        case 8039:
        case 8040:
        case 8041:
        case 8200:
        case 9002:
        case 9003:
        case 9004:
        case 9005:
        case 9006:
        case 9007:
        case 9008:
        case 9009:
        case 9010:
        case 9014:
        case 9015:
        case 9016:
        case 9017:
        case 9018:
        case 9019:
        case 9020:
        case 9021:
        case 10040:
            return 1;
    }

    return 0;
}

#endif
