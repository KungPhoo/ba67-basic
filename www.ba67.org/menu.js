document.addEventListener("DOMContentLoaded", function () {
    const menuItems = [
        { name: "HOME", url: "index.html" },
        { name: "FEATURES", url: "features.html" },
        { name: "DETAILS", url: "readme.html" },
        { name: "FONT", url: "font.html" },
        { name: "PROJECT", url: "https://github.com/KungPhoo/ba67-basic" },
        { name: "RUN", url: "run.html" },
    ];

    const menuDiv = document.getElementById("mobile-menu");
    menuDiv.firstChild.data = "☰ MENU";
    
    const menuList = document.querySelector(".menu-list");

    // Generate menu items dynamically
    menuItems.forEach(item => {
        const li = document.createElement("li");
        const a = document.createElement("a");
        a.href = item.url;
        a.textContent = item.name;
        li.appendChild(a);
        menuList.appendChild(li);
    });

    // Mobile menu toggle
    menuDiv.addEventListener("click", function () {
        menuList.classList.toggle("active");
    });
});
