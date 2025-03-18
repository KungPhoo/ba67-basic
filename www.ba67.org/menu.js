document.addEventListener("DOMContentLoaded", function () {
    const menuItems = [
        { name: "HOME", url: "index.html" },
        { name: "FEATURES", url: "features.html" },
        { name: "DETAILS", url: "readme.html" }
        { name: "PROJECT", url: "https://github.com/KungPhoo/ba67-basic" },
    ];

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
    const menuToggle = document.getElementById("mobile-menu");
    menuToggle.addEventListener("click", function () {
        menuList.classList.toggle("active");
    });
});
