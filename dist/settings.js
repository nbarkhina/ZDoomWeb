var GZDOOMSETTINGS = {
    CLOUDSAVEURL: ""
};

window.GZDOOMSETTINGS = GZDOOMSETTINGS;

function postLoad() {
    if (window["myApp"] == null) return;

    window["myApp"].modReplacementHook = function () {

    };

    window["myApp"].gzDoomIniHook = function (text) {
        return text;
    };
}
window.postLoad = postLoad;
