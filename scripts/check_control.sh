#!/bin/bash

echo "Checking debian/control file..."

# Проверяем, что файл заканчивается новой строкой
if [ "$(tail -c1 debian/control | wc -l)" -eq 0 ]; then
    echo "ERROR: File doesn't end with newline"
    echo "" >> debian/control
    echo "Fixed: Added newline at end of file"
fi

# Проверяем обязательные поля
required_fields=("Package:" "Version:" "Section:" "Priority:" "Architecture:" "Maintainer:" "Description:")

for field in "${required_fields[@]}"; do
    if ! grep -q "^$field" debian/control; then
        echo "ERROR: Missing required field: $field"
        exit 1
    fi
done

echo "debian/control syntax is OK"