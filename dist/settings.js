var GZDOOMSETTINGS = {
    CLOUDSAVEURL: "",

    // --- Autoload ---
    // Example:
    //   WADURL:  "wad/doom2.wad",
    //   MODURLS: "wad/brutal.pk3, wad/myhouse.pk3"
    WADURL: "",
    MODURLS: ""
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
